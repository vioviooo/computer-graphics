#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>
#include <QPainterPath>

class BezierCurveWidget : public QWidget {
    Q_OBJECT

public:
    BezierCurveWidget(QWidget* parent = nullptr) : QWidget(parent), t(0.0), speed(0.001) {
        timer = new QTimer(this);
        // * коннектим сигнал таймера к слоту для обновления позиции
        // connect(timer, &QTimer::timeout, this, &BezierCurveWidget::updatePosition); // * this works as well
        connect(timer, SIGNAL(timeout()), this, SLOT(updatePosition()));
        timer->start(10); // * repaint interval in ms
    }

    void setSpeed(double _speed) {
        speed = _speed; 
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        (void)event; // * since this is not used
        
        QPainter painter(this);

        // * smooth out the edges
        painter.setRenderHint(QPainter::Antialiasing);

        // * bezier curve points

        // * very basic
        // QPointF p0(50, 300);
        // QPointF p1(150, 50);
        // QPointF p2(350, 50);
        // QPointF p3(450, 300);

        //* s curve
        QPointF p0(50, 300);
        QPointF p1(150, 100);
        QPointF p2(300, 500);
        QPointF p3(450, 200);

        // * another curve
        // QPointF p0(50, 300);
        // QPointF p1(150, 200);
        // QPointF p2(350, 400);
        // QPointF p3(450, 300);

        // * draw the curve
        QPainterPath path(p0);
        path.cubicTo(p1, p2, p3);

        painter.setPen(QPen(Qt::green, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);

        // * curr pos of the moving obj
        QPointF currPos = bezierPoint(p0, p1, p2, p3, t);

        // * создаем коэффициент для управления размером фигуры во время движения
        double coef = 1.0 + 2.0 * sin(acos(-1) * t);

        painter.setBrush(Qt::green);

        painter.drawRect(currPos.x() - 20 * coef, currPos.y() - 20 * coef, 40 * coef, 40 * coef);

        painter.setPen(Qt::black);

        QFont font;
        font.setPointSize(20 * coef);
        painter.setFont(font);

        QFontMetrics metrics(font);
        QRect boundingRect = metrics.boundingRect("brat");

        int x = currPos.x() - boundingRect.width() / 2 - 2;
        int y = currPos.y() + boundingRect.height() / 2;

        painter.drawText(x, y, "brat");
    }


private slots:
    void updatePosition() {
        t += speed;
        if (t > 1.0) {
            t = 0.0;
        } else if (t < 0.0) {
            t = 1.0;
        }
        update();  // * start paint event
    }

private:
    double t; // * t param for bezier curve, [0;1]
    double speed; // * animation speed
    QTimer* timer;

    // * calculate bezier func
    QPointF bezierPoint(QPointF p0, QPointF p1, QPointF p2, QPointF p3, double t) {
        double b = 1 - t;
        QPointF point = b * b * b * p0 + 3 * b * b * t * p1 + 3 * b * t * t * p2 + t * t * t * p3;
        // * P(t) = (1-t)^3 * P0 + 3 * (1-t)^2 * t * P1 + 3 * (1-t) * t^2 * P2 + t^3 * P3 ~ Bezier formula
        return point;
    }
};

class OutputWindow : public QWidget {
    Q_OBJECT

private:
    BezierCurveWidget* bezierWidget;

public:
    OutputWindow(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        bezierWidget = new BezierCurveWidget(this);

        QSlider* speedSlider = new QSlider(Qt::Horizontal, this);
        speedSlider->setRange(1, 100);
        speedSlider->setValue(50);

        // * connect the slider to the widget
        connect(speedSlider, &QSlider::valueChanged, this, &OutputWindow::updateSpeed);
        // connect(speedSlider, SIGNAL(valueChanged()), this, SLOT(updateSpeed())); //* doesn;t work because SIGNAL() SLOT() do not support pointer to member syntax

        layout->addWidget(bezierWidget);
        layout->addWidget(speedSlider);

        setLayout(layout);
    }

private slots:
    void updateSpeed(int value) {
        double speed = value / 5000.0;
        bezierWidget->setSpeed(speed);
    }
};

int main(int argc, char** argv) {

    QApplication app(argc, argv);

    OutputWindow window;
    window.resize(500, 650);
    window.show();

    return app.exec(); // * запуск обработки событий (event loop)
}

#include "main.moc"