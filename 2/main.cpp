#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <cmath>
#include <vector>

struct Vertex3D {
    double x, y, z;
};

struct Face {
    std::vector<int> vertices;
};

class ShadowRenderer : public QWidget {
private:
    std::vector<Vertex3D> vertices;
    std::vector<Face> faces;
    Vertex3D lightSource;

public:
    ShadowRenderer(QWidget* parent = nullptr) : QWidget(parent) {
        vertices = {
            {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
            {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
        };

        faces = {
            Face{{0, 1, 2, 3}}, Face{{4, 5, 6, 7}},
            Face{{0, 1, 5, 4}}, Face{{1, 2, 6, 5}},
            Face{{2, 3, 7, 6}}, Face{{3, 0, 4, 7}}
        };


        lightSource = {-12, 0, 5}; // * положение источника света

        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ShadowRenderer::update);
        timer->start(16);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int width = this->width();
        int height = this->height();

        // * центр экрана
        int cx = width / 2;
        int cy = height / 2;

        // * перспективная проекция
        auto project = [&](const Vertex3D& v) -> QPoint {
            double d = 2.0; // Фокусное расстояние
            double x = v.x / (v.z / d + 1) * 100;
            double y = v.y / (v.z / d + 1) * 100;
            return QPoint(cx + x, cy - y);
        };

        // * отрисовка граней с затенением
        for (const auto& face : faces) {
            // * нормаль к грани
            Vertex3D v1 = vertices[face.vertices[1]];
            Vertex3D v0 = vertices[face.vertices[0]];
            Vertex3D v2 = vertices[face.vertices[2]];

            Vertex3D normal = {
                (v1.y - v0.y) * (v2.z - v0.z) - (v1.z - v0.z) * (v2.y - v0.y),
                (v1.z - v0.z) * (v2.x - v0.x) - (v1.x - v0.x) * (v2.z - v0.z),
                (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x),
            };

            // * вектор к источнику света
            Vertex3D lightVector = {
                lightSource.x - v0.x,
                lightSource.y - v0.y,
                lightSource.z - v0.z,
            };

            // * нормализация векторов
            auto normalize = [](Vertex3D& v) {
                double length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
                v.x /= length;
                v.y /= length;
                v.z /= length;
            };

            normalize(normal);
            normalize(lightVector);

            // * cos угла между нормалью и световым вектором
            double brightness = normal.x * lightVector.x + normal.y * lightVector.y + normal.z * lightVector.z;

            brightness = std::max(0.0, brightness); // * освещение только с одной сторон

            int green = static_cast<int>(brightness * 255);
            green = std::max(green, 50); // * минимальное значение для зеленого (темно-зеленый)
            painter.setBrush(QColor(0, green, 0));
            painter.setPen(Qt::NoPen);

            // * проекция граней
            QPolygon polygon;
            for (int index : face.vertices) {
                polygon << project(vertices[index]);
            }

            painter.drawPolygon(polygon);
        }

        QPoint lightPosition = project(lightSource);
        painter.setBrush(Qt::yellow);
        painter.setPen(Qt::black);
        painter.drawEllipse(lightPosition, 5, 5);
    }

    void update() {
        this->repaint();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ShadowRenderer renderer;
    renderer.resize(800, 600);
    renderer.show();

    return app.exec();
}