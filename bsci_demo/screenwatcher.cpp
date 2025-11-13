#include "screenwatcher.h"
#include <testkit.h>

static const int MAX_WIDTH  = 1920;
static const int MAX_HEIGHT = 1080;
static const double MAX_RATE = 30.5;

screenwatcher::screenwatcher(QObject *parent)
    : QObject{parent}
{

    connect(qApp, &QGuiApplication::screenAdded, this, &screenwatcher::onScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &screenwatcher::onScreenRemoved);

    qDebug() << "[ScreenWatcher] platform =" << QGuiApplication::platformName();

    // startup policy
    const auto screens = QGuiApplication::screens();
    for (QScreen* s : screens) {
        applyPolicyForScreen(s);
    }

    m_timer.setInterval(5000);
       connect(&m_timer, &QTimer::timeout,
               this, &screenwatcher::onPollTimer);
       m_timer.start();
}

void screenwatcher::onScreenAdded(QScreen* screen)
{
    qDebug() << "[ScreenWatcher] Screen added:" << screen->name()
             << screen->geometry();
    applyPolicyForScreen(screen);
}

void screenwatcher::onScreenRemoved(QScreen* screen)
{
    qDebug() << "[ScreenWatcher] Screen removed:" << screen->name();
}

void screenwatcher::applyPolicyForScreen(QScreen* screen)
{
    // QScreen::name
    QString outputName = screen->name();
    qDebug() << "[ScreenWatcher] Apply policy for output:" << outputName;

    if (!setBestModeForOutput(outputName)) {
        qWarning() << "[ScreenWatcher] No suitable <= 1080p30 mode found, fallback to auto.";

        QProcess::execute("xrandr", { "--output", outputName, "--auto" });
    }
}

bool screenwatcher::setBestModeForOutput(const QString& outputName)
{
    QProcess proc;
    proc.start("xrandr", QStringList());
    if (!proc.waitForFinished(5000)) {
        qWarning() << "[ScreenWatcher] xrandr timeout";
        return false;
    }

    QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
    if (out.isEmpty()) {
        qWarning() << "[ScreenWatcher] xrandr output empty";
        return false;
    }

    struct Mode {
        int     w;
        int     h;
        double  rate;
    };

    QList<Mode> modes;

    QRegularExpression reHeader(
        QStringLiteral(R"(^%1\s+connected\b.*$)")
        .arg(QRegularExpression::escape(outputName))
    );

    QRegularExpression reModeLine(
        QStringLiteral(R"(^\s+(\S+)\s+(.+)$)")
    );

    QRegularExpression reWH(
        QStringLiteral(R"((\d+)x(\d+))")
    );

    QRegularExpression reRate(
        QStringLiteral(R"((\d+(?:\.\d+)?))")
    );

    bool inTargetBlock = false;
    const QStringList lines = out.split('\n');

    for (const QString& line : lines) {
        if (!inTargetBlock) {
            auto mh = reHeader.match(line);
            if (mh.hasMatch()) {
                inTargetBlock = true;
                continue;
            }
        } else {
            if (!line.startsWith(' ')) {
                break;
            }

            auto mm = reModeLine.match(line);
            if (!mm.hasMatch())
                continue;

            QString modeName = mm.captured(1);
            QString rest     = mm.captured(2);

            auto mwh = reWH.match(modeName);
            if (!mwh.hasMatch())
                continue;

            int w = mwh.captured(1).toInt();
            int h = mwh.captured(2).toInt();

            auto it = reRate.globalMatch(rest);
            while (it.hasNext()) {
                auto rm = it.next();
                double rate = rm.captured(1).toDouble();
                modes.append(Mode{ w, h, rate });

                qDebug() << "[ScreenWatcher] Parsed mode"
                         << w << "x" << h << "@" << rate;
            }
        }
    }

    if (modes.isEmpty()) {
        qWarning() << "[ScreenWatcher] No modes parsed for" << outputName;
        return false;
    }

    static const int    MAX_WIDTH_1080  = 1920;
    static const int    MAX_HEIGHT_1080 = 1080;
    static const double MAX_RATE_1080P  = 30.5;

    auto pickBest = [](const QList<Mode>& list, Mode& bestOut) -> bool {
        if (list.isEmpty())
            return false;
        bool first = true;
        Mode best{};
        for (const Mode& md : list) {
            if (first) {
                best = md;
                first = false;
                continue;
            }

            if (md.h > best.h ||
                (md.h == best.h && md.w > best.w) ||
                (md.h == best.h && md.w == best.w && md.rate > best.rate))
            {
                best = md;
            }
        }
        bestOut = best;
        return true;
    };

    // =============================
    // 1) first priority find 1080p30
    // =============================
    QList<Mode> modes1080p30;
    for (const Mode& md : modes) {
        if (md.w == 1920 && md.h == 1080 && md.rate <= MAX_RATE_1080P) {
            modes1080p30.append(md);
        }
    }

    Mode finalMode{};
    bool hasFinal = false;

    if (!modes1080p30.isEmpty()) {
        Mode best1080{};
        pickBest(modes1080p30, best1080);
        finalMode = best1080;
        hasFinal = true;
        qDebug() << "[ScreenWatcher] Found 1920x1080 <=30Hz mode:"
                 << finalMode.w << "x" << finalMode.h << "@" << finalMode.rate;
    } else {
        // ===========================================
        // 2) Fallback：
        // ===========================================
        QList<Mode> candLe1080;
        QList<Mode> candOthers;

        for (const Mode& md : modes) {
            if (md.w == 1920 && md.h == 1080)
                continue;

            if (md.w <= MAX_WIDTH_1080 && md.h <= MAX_HEIGHT_1080) {
                candLe1080.append(md);
            } else {
                candOthers.append(md);
            }
        }

        Mode bestOther{};
        if (pickBest(candLe1080, bestOther)) {
            finalMode = bestOther;
            hasFinal = true;
            qDebug() << "[ScreenWatcher] Fallback mode (<=1080p, any Hz):"
                     << finalMode.w << "x" << finalMode.h << "@" << finalMode.rate;
        } else if (pickBest(candOthers, bestOther)) {
            finalMode = bestOther;
            hasFinal = true;
            qDebug() << "[ScreenWatcher] Fallback mode (>1080p but not 1920x1080):"
                     << finalMode.w << "x" << finalMode.h << "@" << finalMode.rate;
        }
    }

    if (!hasFinal) {
        qWarning() << "[ScreenWatcher] No final mode selected for" << outputName;
        return false;
    }

    QString modeStr = QString("%1x%2").arg(finalMode.w).arg(finalMode.h);

    QStringList args;
    args << "--output" << outputName
         << "--mode"   << modeStr;

    if (finalMode.w == 1920 && finalMode.h == 1080 && finalMode.rate <= MAX_RATE_1080P) {
        QString rateStr = QString::number(finalMode.rate, 'f', 2);
        args << "--rate" << rateStr;
    }

    qDebug() << "[ScreenWatcher] xrandr args =" << args;

    int ret = QProcess::execute("xrandr", args);

    if (ret != 0) {
        qWarning() << "[ScreenWatcher] xrandr set mode failed, code =" << ret
                   << "output =" << outputName
                   << "args ="   << args;
        return false;
    }

    qDebug() << "[ScreenWatcher] Applied mode for" << outputName << args;
    return true;
}

void screenwatcher::onPollTimer()
{
    const auto screens = QGuiApplication::screens();
    for (QScreen* s : screens) {
        applyPolicyForScreen(s);
    }
}
