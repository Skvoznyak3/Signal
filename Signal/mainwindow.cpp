#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // создаём график
    customPlot = new QCustomPlot(this);
    setCentralWidget(customPlot);

    setupPlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPlot()
{
    QVector<double> x(1000), y(1000);
    for (int i = 0; i < 1000; ++i) {
        x[i] = i / 100.0;                 // время
        y[i] = std::sin(2 * M_PI * x[i]); // синус
    }

    customPlot->addGraph();
    customPlot->graph(0)->setData(x, y);
    customPlot->xAxis->setLabel("Время");
    customPlot->yAxis->setLabel("Амплитуда");
    customPlot->rescaleAxes();
    customPlot->replot();
}
