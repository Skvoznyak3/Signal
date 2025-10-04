#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QDockWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QByteArray>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // Charts for time and frequency domains
    QtCharts::QChartView *timeChartView {nullptr};
    QtCharts::QChart *timeChart {nullptr};
    QtCharts::QLineSeries *timeSeries {nullptr};
    QtCharts::QValueAxis *timeAxisX {nullptr};
    QtCharts::QValueAxis *timeAxisY {nullptr};

    QtCharts::QChartView *freqChartView {nullptr};
    QtCharts::QChart *freqChart {nullptr};
    QtCharts::QLineSeries *freqSeries {nullptr};
    QtCharts::QValueAxis *freqAxisX {nullptr};
    QtCharts::QValueAxis *freqAxisY {nullptr};

    // Data and parameters
    QVector<double> samples;      // normalized to [-1, 1]
    double sampleRateHz {44100.0};
    int fftSize {1024};           // power-of-two

    // Dock for signal generation
    QDockWidget *generatorDock {nullptr};
    QComboBox *waveformCombo {nullptr};
    QDoubleSpinBox *amplitudeSpin {nullptr};
    QDoubleSpinBox *durationSpinSec {nullptr};
    QDoubleSpinBox *sampleRateSpin {nullptr};
    QSpinBox *periodMultipleSpin {nullptr}; // period = 1024 * value (in samples)
    QPushButton *generateButton {nullptr};

    // Actions
    QAction *openAction {nullptr};
    QAction *saveAction {nullptr};
    QAction *saveAsAction {nullptr};
    QAction *resetZoomAction {nullptr};
    QComboBox *fftSizeCombo {nullptr};

    // Setup helpers
    void setupMenusAndToolbar();
    void setupCentralCharts();
    void setupGeneratorDock();

    // Plot updates
    void updateTimePlot();
    void updateSpectrumPlot();

    // File I/O helpers
    bool loadBinaryFile(const QString &filePath);
    bool saveBinaryFile(const QString &filePath);

    // Signal generation and FFT
    void generateSignal();
    void computeFftMagnitude(const QVector<double> &input, int nFft, QVector<double> &outMagnitude);

private slots:
    void onOpen();
    void onSave();
    void onSaveAs();
    void onGenerateClicked();
    void onFftSizeChanged(int index);
    void onResetZoom();
};
#endif // MAINWINDOW_H
