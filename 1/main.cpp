#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>
#include <QPainterPath>

class BezierWidget : public QWidget {
    Q_OBJECT

public:
    BezierWidget(QWidget* parent = nullptr) : QWidget(parent), t(0.0), speed(0.001) {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &BezierWidget::updatePosition);
        timer->start(16); // * ~60 FPS
    }

    void setSpeed(double newSpeed) {
        speed = newSpeed;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        (void)event;
        QPainter painter(this);
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
        painter.setPen(QPen(Qt::green, 2));
        painter.drawPath(path);

        // * curr pos of the moving obj
        QPointF currentPos = bezierPoint(p0, p1, p2, p3, t);

        // * создаем переменную для управления размером фигуры во время движения
        double scale = 1.0 + 2.0 * std::sin(acos(-1) * t);

        painter.setBrush(Qt::green);
        painter.drawRect(currentPos.x() - 20 * scale, currentPos.y() - 20 * scale, 40 * scale, 40 * scale);

        painter.setPen(Qt::black);

        QFont font;
        font.setPointSize(20 * scale);
        painter.setFont(font);

        QFontMetrics metrics(font);
        QRect boundingRect = metrics.boundingRect("brat");

        int x = currentPos.x() - boundingRect.width() / 2 - 2;
        int y = currentPos.y() + boundingRect.height() / 2;

        painter.drawText(x, y, "brat");
    }


private slots:
    void updatePosition() {
        t += speed;
        if (t > 1.0)
            t = 0.0;
        update();  // * start paint event
    }

private:
    double t; // * t param for bezier curve, [0;1]
    double speed; // * animation speed
    QTimer* timer;

    // * calculate bezier func
    QPointF bezierPoint(QPointF p0, QPointF p1, QPointF p2, QPointF p3, double t) {
        double u = 1 - t;
        double tt = t * t;
        double uu = u * u;
        double uuu = uu * u;
        double ttt = tt * t;

        QPointF point = uuu * p0;
        point += 3 * uu * t * p1;
        point += 3 * u * tt * p2;
        point += ttt * p3;
        return point;
    }
};

class outputWindow : public QWidget {
    Q_OBJECT

public:
    outputWindow(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        bezierWidget = new BezierWidget(this);

        QSlider* speedSlider = new QSlider(Qt::Horizontal, this);
        speedSlider->setRange(1, 100);
        speedSlider->setValue(50);

        // * connect the slider to the widget
        connect(speedSlider, &QSlider::valueChanged, this, &outputWindow::updateSpeed);

        layout->addWidget(bezierWidget);
        layout->addWidget(speedSlider);

        setLayout(layout);
    }

private slots:
    void updateSpeed(int value) {
        double speed = value / 5000.0;
        bezierWidget->setSpeed(speed);
    }

private:
    BezierWidget* bezierWidget;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    outputWindow window;
    window.resize(500, 650);
    window.show();

    return app.exec();
}

#include "main.moc"