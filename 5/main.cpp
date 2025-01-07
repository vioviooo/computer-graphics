#include <QtWidgets/QApplication>
#include <QPainter>
#include <QImage>
#include <QWidget>
#include <QVector3D>
#include <cmath>
#include <QKeyEvent>

struct Sphere {
    QVector3D center;
    float radius;
    QColor color;
    bool isFocused;
};

class DepthOfFieldWidget : public QWidget {
public:
    DepthOfFieldWidget(QWidget* parent = nullptr) : QWidget(parent), focusDistance(8.0f), depthOfField(2.0f) {
        setWindowTitle("Depth of Field");
        setFixedSize(800, 600);
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        QImage image(width(), height(), QImage::Format_RGB32);
        image.fill(Qt::black);

        QVector3D cameraPos(0, 0, 0);

        QVector<Sphere> spheres = {
            {QVector3D(-2, -0.5, 13), 1.2f, QColor(255, 0, 0), false},
            {QVector3D(2, 0.5, 13), 1.2f, QColor(0, 255, 0), false},
            {QVector3D(0, 0, 12), 1.5f, QColor(0, 0, 255), false}
        };

        QVector3D lightPos(5, 5, 0);
        QColor lightColor(255, 255, 255);

        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                QVector3D rayDir = QVector3D(x - width() / 2.0f, y - height() / 2.0f, 800).normalized();
                QColor pixelColor = traceRay(cameraPos, rayDir, spheres, lightPos, lightColor);
                image.setPixelColor(x, y, pixelColor);
            }
        }
        painter.drawImage(0, 0, image);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Up) {
            focusDistance += 0.1f;
        }
        if (event->key() == Qt::Key_Down) {
            focusDistance -= 0.1f;
        }
        update();
    }

private:
    float focusDistance;
    float depthOfField;

    QColor traceRay(const QVector3D& cameraPos, const QVector3D& rayDir, const QVector<Sphere>& spheres,
                    const QVector3D& lightPos, const QColor& lightColor) {
        QColor resultColor(0, 0, 0);
        float minT = std::numeric_limits<float>::max();

        for (const auto& sphere : spheres) {
            float t = intersectRaySphere(cameraPos, rayDir, sphere);
            if (t > 0 && t < minT) {
                minT = t;
                QVector3D intersection = cameraPos + rayDir * t;
                QVector3D normal = (intersection - sphere.center).normalized();
                QVector3D lightDir = (lightPos - intersection).normalized();
                QVector3D reflectDir = (2.0f * QVector3D::dotProduct(normal, lightDir) * normal - lightDir).normalized();

                float diff = std::max(QVector3D::dotProduct(normal, lightDir), 0.0f);
                float specular = std::pow(std::max(QVector3D::dotProduct(reflectDir, -rayDir), 0.0f), 32);

                QColor illuminatedColor = sphere.color;
                illuminatedColor.setRed(std::min(int(sphere.color.red() * diff + specular * lightColor.red()), 255));
                illuminatedColor.setGreen(std::min(int(sphere.color.green() * diff + specular * lightColor.green()), 255));
                illuminatedColor.setBlue(std::min(int(sphere.color.blue() * diff + specular * lightColor.blue()), 255));

                float blurFactor = std::abs((intersection - cameraPos).length() - focusDistance) / depthOfField;
                blurFactor = std::clamp(blurFactor, 0.2f, 1.0f);

                resultColor.setRed(resultColor.red() * blurFactor + illuminatedColor.red() * (1 - blurFactor));
                resultColor.setGreen(resultColor.green() * blurFactor + illuminatedColor.green() * (1 - blurFactor));
                resultColor.setBlue(resultColor.blue() * blurFactor + illuminatedColor.blue() * (1 - blurFactor));
            }
        }

        return resultColor;
    }

    float intersectRaySphere(const QVector3D& rayOrigin, const QVector3D& rayDir, const Sphere& sphere) {
        QVector3D oc = rayOrigin - sphere.center;
        float a = QVector3D::dotProduct(rayDir, rayDir);
        float b = 2.0f * QVector3D::dotProduct(oc, rayDir);
        float c = QVector3D::dotProduct(oc, oc) - sphere.radius * sphere.radius;

        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return -1;

        return (-b - std::sqrt(discriminant)) / (2.0f * a);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    DepthOfFieldWidget widget;
    widget.show();
    return app.exec();
}