#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>
#include <QGraphicsView>

using namespace QtCharts;

static bool isPowerOfTwo(int n) { return n >= 1 && (n & (n - 1)) == 0; }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupMenusAndToolbar();
    setupCentralCharts();
    setupGeneratorDock();

    // Seed with a default simple sine signal
    samples.resize(2048);
    const double frequencyHz = 1000.0;
    for (int i = 0; i < samples.size(); ++i) {
        const double t = static_cast<double>(i) / sampleRateHz;
        samples[i] = std::sin(2.0 * M_PI * frequencyHz * t);
    }
    updateTimePlot();
    updateSpectrumPlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMenusAndToolbar()
{
    // File actions
    openAction = new QAction(tr("Открыть"), this);
    saveAction = new QAction(tr("Сохранить"), this);
    saveAsAction = new QAction(tr("Сохранить как"), this);
    resetZoomAction = new QAction(tr("Сбросить масштаб"), this);

    connect(openAction, &QAction::triggered, this, &MainWindow::onOpen);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSave);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::onResetZoom);

    auto fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);

    auto viewMenu = menuBar()->addMenu(tr("Вид"));
    viewMenu->addAction(resetZoomAction);

    // Toolbar with FFT size selector
    auto toolBar = addToolBar(tr("Инструменты"));
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addSeparator();

    toolBar->addWidget(new QLabel(tr("FFT:") + " ", this));
    fftSizeCombo = new QComboBox(this);
    const QList<int> sizes {256, 512, 1024, 2048, 4096, 8192};
    for (int s : sizes) fftSizeCombo->addItem(QString::number(s));
    fftSizeCombo->setCurrentText(QString::number(fftSize));
    connect(fftSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFftSizeChanged);
    toolBar->addWidget(fftSizeCombo);

    toolBar->addSeparator();
    toolBar->addAction(resetZoomAction);
}

void MainWindow::setupCentralCharts()
{
    auto central = new QWidget(this);
    auto vbox = new QVBoxLayout(central);

    // Time domain chart
    timeChart = new QChart();
    timeSeries = new QLineSeries();
    timeAxisX = new QValueAxis();
    timeAxisY = new QValueAxis();
    timeAxisX->setTitleText(tr("Время, с"));
    timeAxisY->setTitleText(tr("Амплитуда"));
    timeChart->addSeries(timeSeries);
    timeChart->addAxis(timeAxisX, Qt::AlignBottom);
    timeChart->addAxis(timeAxisY, Qt::AlignLeft);
    timeSeries->attachAxis(timeAxisX);
    timeSeries->attachAxis(timeAxisY);
    timeChart->legend()->hide();
    timeChartView = new QChartView(timeChart, this);
    timeChartView->setRenderHint(QPainter::Antialiasing);
    timeChartView->setRubberBand(QChartView::RectangleRubberBand);
    timeChartView->setDragMode(QGraphicsView::ScrollHandDrag);

    // Frequency domain chart
    freqChart = new QChart();
    freqSeries = new QLineSeries();
    freqAxisX = new QValueAxis();
    freqAxisY = new QValueAxis();
    freqAxisX->setTitleText(tr("Частота, Гц"));
    freqAxisY->setTitleText(tr("Магнитуда"));
    freqChart->addSeries(freqSeries);
    freqChart->addAxis(freqAxisX, Qt::AlignBottom);
    freqChart->addAxis(freqAxisY, Qt::AlignLeft);
    freqSeries->attachAxis(freqAxisX);
    freqSeries->attachAxis(freqAxisY);
    freqChart->legend()->hide();
    freqChartView = new QChartView(freqChart, this);
    freqChartView->setRenderHint(QPainter::Antialiasing);
    freqChartView->setRubberBand(QChartView::RectangleRubberBand);
    freqChartView->setDragMode(QGraphicsView::ScrollHandDrag);

    vbox->addWidget(timeChartView, 1);
    vbox->addWidget(freqChartView, 1);
    setCentralWidget(central);
}

void MainWindow::setupGeneratorDock()
{
    generatorDock = new QDockWidget(tr("Генератор сигналов"), this);
    auto w = new QWidget(generatorDock);
    auto form = new QFormLayout(w);

    waveformCombo = new QComboBox(w);
    waveformCombo->addItem(tr("Синус"));
    waveformCombo->addItem(tr("Пила"));
    waveformCombo->addItem(tr("Прямоугольный"));
    waveformCombo->addItem(tr("Случайный"));

    amplitudeSpin = new QDoubleSpinBox(w);
    amplitudeSpin->setRange(0.0, 1.0);
    amplitudeSpin->setSingleStep(0.05);
    amplitudeSpin->setValue(1.0);

    durationSpinSec = new QDoubleSpinBox(w);
    durationSpinSec->setRange(0.01, 60.0);
    durationSpinSec->setSingleStep(0.01);
    durationSpinSec->setValue(0.1);

    sampleRateSpin = new QDoubleSpinBox(w);
    sampleRateSpin->setRange(100.0, 192000.0);
    sampleRateSpin->setDecimals(0);
    sampleRateSpin->setSingleStep(100.0);
    sampleRateSpin->setValue(sampleRateHz);

    periodMultipleSpin = new QSpinBox(w);
    periodMultipleSpin->setRange(1, 1024);
    periodMultipleSpin->setValue(1);

    generateButton = new QPushButton(tr("Сгенерировать"), w);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);

    form->addRow(tr("Форма"), waveformCombo);
    form->addRow(tr("Амплитуда"), amplitudeSpin);
    form->addRow(tr("Длительность, с"), durationSpinSec);
    form->addRow(tr("Частота дискретизации, Гц"), sampleRateSpin);
    form->addRow(tr("Период = 1024 ×"), periodMultipleSpin);
    form->addWidget(generateButton);

    w->setLayout(form);
    generatorDock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, generatorDock);
}

void MainWindow::updateTimePlot()
{
    timeSeries->clear();
    if (samples.isEmpty()) return;
    QList<QPointF> points;
    points.reserve(samples.size());
    for (int i = 0; i < samples.size(); ++i) {
        points.append(QPointF(static_cast<double>(i) / sampleRateHz, samples[i]));
    }
    timeSeries->replace(points);
    timeAxisX->setRange(0.0, static_cast<double>(samples.size()) / sampleRateHz);
    timeAxisY->setRange(-1.1, 1.1);
}

void MainWindow::updateSpectrumPlot()
{
    if (samples.isEmpty()) {
        freqSeries->clear();
        return;
    }
    const int n = std::min(fftSize, samples.size());
    QVector<double> windowed(n);
    // Hann window to reduce leakage
    for (int i = 0; i < n; ++i) {
        const double w = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (n - 1)));
        windowed[i] = samples[i] * w;
    }
    QVector<double> mag;
    computeFftMagnitude(windowed, n, mag);

    // Single-sided spectrum 0..Nyquist
    const int half = n / 2;
    QList<QPointF> points;
    points.reserve(half);
    for (int k = 0; k < half; ++k) {
        const double freq = (static_cast<double>(k) * sampleRateHz) / n;
        points.append(QPointF(freq, mag[k]));
    }
    freqSeries->replace(points);
    freqAxisX->setRange(0.0, sampleRateHz / 2.0);
    // autoscale Y
    double maxY = 1e-9;
    for (const auto &p : points) maxY = std::max(maxY, p.y());
    freqAxisY->setRange(0.0, maxY * 1.1);
}

bool MainWindow::loadBinaryFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.readAll();
    f.close();
    samples.resize(data.size());
    // Each byte is a sample. Interpret as unsigned 8-bit [0..255] and map to [-1..1]
    for (int i = 0; i < data.size(); ++i) {
        const unsigned char u = static_cast<unsigned char>(data.at(i));
        samples[i] = (static_cast<double>(u) - 128.0) / 127.0;
    }
    updateTimePlot();
    updateSpectrumPlot();
    return true;
}

bool MainWindow::saveBinaryFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QByteArray data;
    data.resize(samples.size());
    for (int i = 0; i < samples.size(); ++i) {
        double v = std::clamp(samples[i], -1.0, 1.0);
        int u = static_cast<int>(std::lround((v * 127.0) + 128.0));
        if (u < 0) u = 0; if (u > 255) u = 255;
        data[i] = static_cast<char>(u);
    }
    const bool ok = f.write(data) == data.size();
    f.close();
    return ok;
}

void MainWindow::generateSignal()
{
    sampleRateHz = sampleRateSpin->value();
    const double amplitude = amplitudeSpin->value();
    const double durationSec = durationSpinSec->value();
    const int periodMultiple = periodMultipleSpin->value();
    const int periodSamples = 1024 * periodMultiple; // must be multiple of 1024
    const int totalSamples = std::max( periodSamples, static_cast<int>(std::lround(durationSec * sampleRateHz)) );
    const int n = ((totalSamples + periodSamples - 1) / periodSamples) * periodSamples; // snap to multiples of period
    samples.resize(n);

    const QString wf = waveformCombo->currentText();
    if (wf.contains("Случай", Qt::CaseInsensitive)) {
        for (int i = 0; i < n; ++i) {
            const double r = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
            samples[i] = amplitude * r;
        }
    } else if (wf.contains("Пила", Qt::CaseInsensitive)) {
        for (int i = 0; i < n; ++i) {
            const double phase = static_cast<double>(i % periodSamples) / periodSamples; // 0..1
            const double saw = 2.0 * (phase) - 1.0; // -1..1
            samples[i] = amplitude * saw;
        }
    } else if (wf.contains("Прямоуголь", Qt::CaseInsensitive)) {
        for (int i = 0; i < n; ++i) {
            const int pos = i % periodSamples;
            const double sq = (pos < periodSamples / 2) ? 1.0 : -1.0;
            samples[i] = amplitude * sq;
        }
    } else { // Синус
        // Sine with integer number of periods in the buffer (frequency derived from periodSamples)
        const double frequency = sampleRateHz / static_cast<double>(periodSamples);
        for (int i = 0; i < n; ++i) {
            const double t = static_cast<double>(i) / sampleRateHz;
            samples[i] = amplitude * std::sin(2.0 * M_PI * frequency * t);
        }
    }

    updateTimePlot();
    updateSpectrumPlot();
}

void MainWindow::computeFftMagnitude(const QVector<double> &input, int nFft, QVector<double> &outMagnitude)
{
    // Simple iterative Cooley–Tukey radix-2 FFT for real input; returns magnitude spectrum
    if (!isPowerOfTwo(nFft)) nFft = 1 << static_cast<int>(std::floor(std::log2(nFft)));
    struct Complex { double re, im; };
    QVector<Complex> a(nFft);
    const int n = std::min(nFft, input.size());
    for (int i = 0; i < n; ++i) { a[i] = { input[i], 0.0 }; }
    for (int i = n; i < nFft; ++i) { a[i] = { 0.0, 0.0 }; }

    // bit-reverse permutation
    int j = 0;
    for (int i = 1; i < nFft; ++i) {
        int bit = nFft >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= nFft; len <<= 1) {
        const double ang = -2.0 * M_PI / static_cast<double>(len);
        const double wlenCos = std::cos(ang);
        const double wlenSin = std::sin(ang);
        for (int i = 0; i < nFft; i += len) {
            double wCos = 1.0;
            double wSin = 0.0;
            for (int k = 0; k < len/2; ++k) {
                Complex u = a[i + k];
                Complex v = { a[i + k + len/2].re * wCos - a[i + k + len/2].im * wSin,
                               a[i + k + len/2].re * wSin + a[i + k + len/2].im * wCos };
                a[i + k].re = u.re + v.re; a[i + k].im = u.im + v.im;
                a[i + k + len/2].re = u.re - v.re; a[i + k + len/2].im = u.im - v.im;
                const double newCos = wCos * wlenCos - wSin * wlenSin;
                const double newSin = wCos * wlenSin + wSin * wlenCos;
                wCos = newCos; wSin = newSin;
            }
        }
    }

    outMagnitude.resize(nFft/2);
    for (int k = 0; k < nFft/2; ++k) {
        const double mag = std::hypot(a[k].re, a[k].im) / (nFft / 2.0);
        outMagnitude[k] = mag;
    }
}

void MainWindow::onOpen()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Открыть бинарный файл"), QString(), tr("Binary (*.bin);;All (*)"));
    if (file.isEmpty()) return;
    if (!loadBinaryFile(file)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось открыть файл"));
    }
}

void MainWindow::onSave()
{
    const QString file = QFileDialog::getSaveFileName(this, tr("Сохранить"), QString(), tr("Binary (*.bin);;All (*)"));
    if (file.isEmpty()) return;
    if (!saveBinaryFile(file)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл"));
    }
}

void MainWindow::onSaveAs()
{
    onSave();
}

void MainWindow::onGenerateClicked()
{
    generateSignal();
}

void MainWindow::onFftSizeChanged(int index)
{
    Q_UNUSED(index);
    const int newSize = fftSizeCombo->currentText().toInt();
    if (isPowerOfTwo(newSize)) {
        fftSize = newSize;
        updateSpectrumPlot();
    }
}

void MainWindow::onResetZoom()
{
    updateTimePlot();
    updateSpectrumPlot();
}
