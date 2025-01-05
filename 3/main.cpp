#include <QApplication>
#include <QOpenGLWidget>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QVector3D>
#include <QOpenGLFunctions>
#include <QMatrix4x4>

class Camera {
public:
    QVector3D position;
    QVector3D targetPosition;
    float speed;

    Camera(const QVector3D &startPos)
        : position(startPos), targetPosition(startPos), speed(0.01f) {}

    void updatePosition() {
        position += (targetPosition - position) * speed;
    }

    void setTarget(const QVector3D &target) {
        targetPosition = target;
    }
};

class Scene : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Scene(QWidget *parent = nullptr) : QOpenGLWidget(parent), camera(QVector3D(0, 0, 5)) {
        setFocusPolicy(Qt::StrongFocus);
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Scene::updateScene);
        timer->start(16);
    }

    void setSpeed(float newSpeed) {
        camera.speed = newSpeed;
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);

        QMatrix4x4 projection;
        projection.perspective(60.0f, float(w) / h, 0.1f, 100.0f);
        projectionMatrix = projection;
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 viewMatrix;
        viewMatrix.lookAt(camera.position, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

        QMatrix4x4 modelView = projectionMatrix * viewMatrix;

        QMatrix4x4 cubeMatrix = modelView;
        cubeMatrix.translate(-3.0f, 0.0f, 0.0f);
        drawCube(cubeMatrix);

        QMatrix4x4 pyramidMatrix = modelView;
        pyramidMatrix.translate(0.0f, 0.0f, 0.0f);
        drawPyramid(pyramidMatrix);

        QMatrix4x4 sphereMatrix = modelView;
        sphereMatrix.translate(3.0f, 0.0f, 0.0f);
        drawSphere(sphereMatrix);
    }

    void keyPressEvent(QKeyEvent *event) override {
        qDebug() << "Key Pressed:" << event->key();
        if (event->key() == Qt::Key_1) {
            camera.setTarget(QVector3D(0, 0, 5));
        } else if (event->key() == Qt::Key_2) {
            camera.setTarget(QVector3D(5, 5, 5));
        } else if (event->key() == Qt::Key_3) {
            camera.setTarget(QVector3D(-5, -5, 5));
        } else if (event->key() == Qt::Key_4) {
            camera.setTarget(QVector3D(10, 0, 5));
        } else if (event->key() == Qt::Key_5) {
            camera.setTarget(QVector3D(-10, 0, 5));
        }
    }

private slots:
    void updateScene() {
        camera.updatePosition();
        update();
    }

private:
    Camera camera;
    QTimer *timer;
    QMatrix4x4 projectionMatrix;

    void drawCube(const QMatrix4x4 &matrix) {
        glLoadMatrixf(matrix.constData());

        glBegin(GL_QUADS);

        // * передняя грань
        glColor3f(1.0f, 0.0f, 0.0f); // * rрасный
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);

        // * задняя грань
        glColor3f(0.0f, 1.0f, 0.0f); // * зеленый
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);

        // * левая грань
        glColor3f(0.0f, 0.0f, 1.0f); // * синий
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);

        // * правая грань
        glColor3f(1.0f, 1.0f, 0.0f); // * желтый
        glVertex3f(1.0f, -1.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);

        // * верхняя грань
        glColor3f(0.0f, 1.0f, 1.0f); // * голубой
        glVertex3f(-1.0f, 1.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, -1.0f);

        // * нижняя грань
        glColor3f(1.0f, 0.0f, 1.0f); // * фиол
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);

        glEnd();
    }

    void drawSphere(const QMatrix4x4 &matrix) {
        glLoadMatrixf(matrix.constData());

        glColor3f(0.5f, 0.5f, 1.0f);
        const int slices = 20;
        const int stacks = 20;

        for (int i = 0; i < stacks; ++i) {
            float theta1 = (float(i) / stacks) * M_PI;
            float theta2 = (float(i + 1) / stacks) * M_PI;

            glBegin(GL_TRIANGLE_STRIP);
            for (int j = 0; j <= slices; ++j) {
                float phi = (float(j) / slices) * 2.0f * M_PI;

                float x1 = sin(theta1) * cos(phi);
                float y1 = cos(theta1);
                float z1 = sin(theta1) * sin(phi);

                float x2 = sin(theta2) * cos(phi);
                float y2 = cos(theta2);
                float z2 = sin(theta2) * sin(phi);

                glVertex3f(x1, y1, z1);
                glVertex3f(x2, y2, z2);
            }
            glEnd();
        }
    }

    void drawPyramid(const QMatrix4x4 &matrix) {
        glLoadMatrixf(matrix.constData());

        glBegin(GL_TRIANGLES);

        float apexX = 0.0f, apexY = 1.0f, apexZ = 0.0f;

        float baseVertices[4][3] = {
            {-1.0f, -1.0f, -1.0f},
            {1.0f, -1.0f, -1.0f},
            {1.0f, -1.0f, 1.0f},
            {-1.0f, -1.0f, 1.0f}
        };

        float colors[4][3] = {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f}
        };

        for (int i = 0; i < 4; ++i) {
            glColor3f(colors[i][0], colors[i][1], colors[i][2]);
            glVertex3f(apexX, apexY, apexZ);
            glVertex3f(baseVertices[i][0], baseVertices[i][1], baseVertices[i][2]);
            glVertex3f(baseVertices[(i + 1) % 4][0], baseVertices[(i + 1) % 4][1], baseVertices[(i + 1) % 4][2]);
        }

        glEnd();

        glBegin(GL_QUADS);
        glColor3f(0.5f, 0.5f, 0.5f);
        for (int i = 0; i < 4; ++i) {
            glVertex3f(baseVertices[i][0], baseVertices[i][1], baseVertices[i][2]);
        }
        glEnd();
    }
};

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) {
        auto layout = new QVBoxLayout(this);
        scene = new Scene(this);

        auto slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(1, 100);
        connect(slider, &QSlider::valueChanged, this, [&](int value) {
            scene->setSpeed(value / 100.0f);
        });

        layout->addWidget(scene);
        layout->addWidget(slider);
    }

private:
    Scene *scene;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    qDebug() << "EJFJEFJ\n";
    MainWindow window;
    window.resize(1000, 800);
    window.show();

    return app.exec();
}

#include "main.moc"
