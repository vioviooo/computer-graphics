#include <QtWidgets/QApplication>
#include <QPainter>
#include <QImage>
#include <QWidget>
#include <QVector3D>
#include <cmath>
#include <QKeyEvent>
#include <vector>

struct Sphere {
    QVector3D center;
    float radius;
    QColor color;
    bool isFocused;
};

struct distancedColor {
    float distance;
    QColor color;
};

class DepthOfFieldWidget : public QWidget {
public:
    DepthOfFieldWidget(QWidget* parent = nullptr) : QWidget(parent) {
        focusDistance = 10.0f;
        depthOfField = 8.0f;
        focusStep = 4.0f;
        maxBlurIntensity = 8;
        maxBlackBlurIntensity = 3;

        setWindowTitle("Depth of Field");
        setFixedSize(1000, 900);
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        QImage image(width(), height(), QImage::Format_RGB32);
        image.fill(Qt::black);

        QVector<Sphere> spheres = {
            {QVector3D(-2, -0.5, 6), 1.2f, QColor(255, 0, 0), false},
            {QVector3D(4, 0.5, 14), 1.2f, QColor(0, 255, 0), false},
            {QVector3D(0, 0, 10), 1.5f, QColor(0, 0, 255), false}
        };

        QVector3D cameraPos = QVector3D(0, 0, 0);
        QVector3D lightPos(5, 5, 0);
        QColor lightColor(255, 255, 255);

        distancedColor b;
        b.color = Qt::black;
        std::vector<std::vector<distancedColor>> pixels(width(), std::vector<distancedColor>(height(), b));


        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                QVector3D rayDir = QVector3D(x - width() / 2.0f, y - height() / 2.0f, 800).normalized();
                pixels[x][y] = traceRay(cameraPos, rayDir, spheres, lightPos, lightColor);
            }
        }

        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                QColor color = blur(pixels, x, y);
                image.setPixelColor(x, y, color);
            }
        }

        painter.drawImage(0, 0, image);
        qDebug() << "drawn";
    }

    void keyPressEvent(QKeyEvent* event) override {
        qDebug() << "key pressed:" << event->key();
        if (event->key() == Qt::Key_Up) {
            focusDistance += focusStep;
            qDebug() << "focus distance:" << focusDistance;
        }
        if (event->key() == Qt::Key_Down) {
            focusDistance -= focusStep;
            if (focusDistance < 0) {
                focusDistance = 0;
            }
            qDebug() << "focus distance:" << focusDistance;
        }
        update();
    }

private:
    float focusDistance;
    float depthOfField;
    float focusStep;
    int maxBlurIntensity;
    int maxBlackBlurIntensity;

    QColor blur(const std::vector<std::vector<distancedColor>>& pixels, int x, int y) {
        float blurFactor = std::abs(pixels[x][y].distance - focusDistance) / depthOfField;
        blurFactor = std::clamp(blurFactor, 0.0f, 1.0f);
        int blurIntensity;

        if (pixels[x][y].color != Qt::black) {
            blurIntensity = std::round(maxBlurIntensity * blurFactor);
        } else {
            blurIntensity = std::round(maxBlackBlurIntensity * blurFactor);
        }

        int r = blurIntensity;
        int pixelCnt = 0;
        int red   = 0;
        int green = 0;
        int blue  = 0;

        for (int i = std::max(x-r, 0); i < std::min(x+r+1, width()); ++i) {
            for (int j = std::max(y-r, 0); j < std::min(y+r+1, height()); ++j) {
                red   += pixels[i][j].color.red();
                green += pixels[i][j].color.green();
                blue  += pixels[i][j].color.blue();
                pixelCnt++;
            }
        }

        red   = std::clamp((int) std::round(red   / pixelCnt), 0, 255);
        green = std::clamp((int) std::round(green / pixelCnt), 0, 255);
        blue  = std::clamp((int) std::round(blue  / pixelCnt), 0, 255);

        return QColor(red, green, blue);
    }

    distancedColor traceRay(const QVector3D& cameraPos, const QVector3D& rayDir, const QVector<Sphere>& spheres,
                    const QVector3D& lightPos, const QColor& lightColor) {
        distancedColor result = {std::numeric_limits<float>::max(), Qt::black};
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

                result.color.setRed(std::min(int(sphere.color.red() * diff + specular * lightColor.red()), 255));
                result.color.setGreen(std::min(int(sphere.color.green() * diff + specular * lightColor.green()), 255));
                result.color.setBlue(std::min(int(sphere.color.blue() * diff + specular * lightColor.blue()), 255));
                result.distance = (intersection - cameraPos).length();
            }

        }

        return result;
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
