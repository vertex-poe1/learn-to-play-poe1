#include <QtTest/QtTest>
#include <QRect>

// Mirrors the physical→logical conversion used in GameOverlay::updateGameRect on Windows.
static QRect physicalToLogical(const QRect &r, qreal dpr)
{
    return QRect(
        qRound(r.x()      / dpr),
        qRound(r.y()      / dpr),
        qRound(r.width()  / dpr),
        qRound(r.height() / dpr)
    );
}

class TestDpi : public QObject
{
    Q_OBJECT
private slots:
    void unity_dpr()
    {
        QRect phys(100, 200, 1920, 1080);
        QCOMPARE(physicalToLogical(phys, 1.0), phys);
    }

    void two_x_dpr()
    {
        QRect phys(200, 400, 3840, 2160);
        QCOMPARE(physicalToLogical(phys, 2.0), QRect(100, 200, 1920, 1080));
    }

    void fractional_125_pct()
    {
        // 125 % DPI (1.25 DPR): 1920×1080 physical → 1536×864 logical
        QRect phys(0, 0, 1920, 1080);
        QRect result = physicalToLogical(phys, 1.25);
        QCOMPARE(result.width(),  1536);
        QCOMPARE(result.height(), 864);
    }

    void fractional_150_pct_with_offset()
    {
        // 150 % DPI (1.5 DPR): origin (100, 50) physical → (67, 33) logical
        QRect phys(100, 50, 2880, 1620);
        QRect result = physicalToLogical(phys, 1.5);
        QCOMPARE(result.x(),      67);
        QCOMPARE(result.y(),      33);
        QCOMPARE(result.width(),  1920);
        QCOMPARE(result.height(), 1080);
    }
};

QTEST_MAIN(TestDpi)
#include "test_dpi.moc"
