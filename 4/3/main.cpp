#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QtMath>

class Scene : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Scene(QWidget *parent = nullptr) : QOpenGLWidget(parent), rotationAngle(0.0f) {
        setFocusPolicy(Qt::StrongFocus);
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Scene::onTimeout);
        timer->start(16);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
        initShaders();
        setupCube();
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        projection.setToIdentity();
        projection.perspective(45.0f, float(w) / float(h), 0.1f, 100.0f);
    }

   void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 view;
        view.lookAt(cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

        shaderProgram.bind();
        shaderProgram.setUniformValue("lightPos", lightPosition);
        shaderProgram.setUniformValue("lightDir", lightDirection);
        shaderProgram.setUniformValue("cameraPos", cameraPosition);

        // Draw cube
        QMatrix4x4 cubeModel;
        cubeModel.translate(-1.5f, 0.0f, -3.0f);
        cubeModel.rotate(rotationAngle, 0.5f, 1.0f, 0.0f);
        QMatrix4x4 cubeMVP = projection * view * cubeModel;
        shaderProgram.setUniformValue("mvp", cubeMVP);
        drawCube();

        // Draw sphere
        QMatrix4x4 sphereModel;
        sphereModel.translate(1.5f, 0.0f, -3.0f);
        sphereModel.rotate(rotationAngle, 0.5f, 1.0f, 0.0f);
        QMatrix4x4 sphereMVP = projection * view * sphereModel;
        shaderProgram.setUniformValue("mvp", sphereMVP);
        drawSphere();

        // Draw pyramid
        QMatrix4x4 pyramidModel;
        pyramidModel.translate(0.0f, 0.0f, -3.0f);
        pyramidModel.rotate(rotationAngle, 0.5f, 1.0f, 0.0f);
        QMatrix4x4 pyramidMVP = projection * view * pyramidModel;
        shaderProgram.setUniformValue("mvp", pyramidMVP);
        drawPyramid();

        GLint positionAttr = shaderProgram.attributeLocation("position");
        GLint normalAttr = shaderProgram.attributeLocation("normal");
        qDebug() << "position attr:" << positionAttr << "normal attr:" << normalAttr;

        shaderProgram.bind();

        GLuint error = glGetError();
        if (error != GL_NO_ERROR) {
            qDebug() << "OpenGL error:" << error;
        }

        shaderProgram.release();
    }

    void keyPressEvent(QKeyEvent *event) override {
        qDebug() << "Key Pressed:" << event->key();
        if (event->key() == Qt::Key_W) {
            cameraPosition.setZ(cameraPosition.z() - 2.0f);
        } else if (event->key() == Qt::Key_S) {
            cameraPosition.setZ(cameraPosition.z() + 2.0f);
        }
        update();
    }

private slots:
    void onTimeout() {
        rotationAngle += 1.0f;
        if (rotationAngle >= 360.0f) {
            rotationAngle -= 360.0f;
        }
        update();
    }

private:
    QTimer *timer;
    QMatrix4x4 projection;
    QOpenGLShaderProgram shaderProgram;
    QVector3D lightPosition = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D lightDirection = QVector3D(-0.5f, -1.0f, -0.5f);
    QVector3D cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
    GLuint cubeVAO, cubeVBO;
    GLuint pyramidVAO, pyramidVBO;

    float rotationAngle;

    void initShaders() {
        const char *vertexShaderSource = R"(
            #version 120
            attribute vec3 position;
            attribute vec3 normal;
            uniform mat4 mvp;
            uniform vec3 lightPos;
            uniform vec3 lightDir;
            uniform vec3 cameraPos;
            varying vec3 fragColor;

            void main() {
                gl_Position = mvp * vec4(position, 1.0);
                vec3 ambient = 0.1 * vec3(1.0, 1.0, 1.0);
                vec3 norm = normalize(normal);
                vec3 lightDirNormalized = normalize(lightPos - vec3(gl_Position));
                float diff = max(dot(norm, lightDirNormalized), 0.0);
                vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
                fragColor = ambient + diffuse;
            }
        )";

        const char *fragmentShaderSource = R"(
            #version 120
            varying vec3 fragColor;
            void main() {
                gl_FragColor = vec4(fragColor, 1.0);
            }
        )";

        if (!shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
            qDebug() << "Error compiling vertex shader:" << shaderProgram.log();
            return;
        }

        if (!shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
            qDebug() << "Error compiling fragment shader:" << shaderProgram.log();
            return;
        }

        if (!shaderProgram.link()) {
            qDebug() << "Error linking shader program:" << shaderProgram.log();
            return;
        }

        if (!shaderProgram.bind()) {
            qDebug() << "Error binding shader program:" << shaderProgram.log();
        }
    }

    void drawSphere() {
        const int slices = 20;
        const int stacks = 20;

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < stacks; ++i) {
            float theta1 = float(i) / stacks * M_PI;
            float theta2 = float(i + 1) / stacks * M_PI;

            for (int j = 0; j < slices; ++j) {
                float phi1 = float(j) / slices * 2 * M_PI;
                float phi2 = float(j + 1) / slices * 2 * M_PI;

                QVector3D v1(sin(theta1) * cos(phi1), cos(theta1), sin(theta1) * sin(phi1));
                QVector3D v2(sin(theta2) * cos(phi1), cos(theta2), sin(theta2) * sin(phi1));
                QVector3D v3(sin(theta2) * cos(phi2), cos(theta2), sin(theta2) * sin(phi2));
                QVector3D v4(sin(theta1) * cos(phi2), cos(theta1), sin(theta1) * sin(phi2));

                glVertexAttrib3f(1, v1.x(), v1.y(), v1.z());
                glVertex3f(v1.x(), v1.y(), v1.z());

                glVertexAttrib3f(1, v2.x(), v2.y(), v2.z());
                glVertex3f(v2.x(), v2.y(), v2.z());

                glVertexAttrib3f(1, v3.x(), v3.y(), v3.z());
                glVertex3f(v3.x(), v3.y(), v3.z());

                glVertexAttrib3f(1, v1.x(), v1.y(), v1.z());
                glVertex3f(v1.x(), v1.y(), v1.z());

                glVertexAttrib3f(1, v3.x(), v3.y(), v3.z());
                glVertex3f(v3.x(), v3.y(), v3.z());

                glVertexAttrib3f(1, v4.x(), v4.y(), v4.z());
                glVertex3f(v4.x(), v4.y(), v4.z());
            }
        }
        glEnd();
    }

     void setupCube() {
        GLfloat vertices[] = {
            -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,
        };

        GLuint indices[] = {
            0, 1, 2,  0, 2, 3,
            4, 5, 6,  4, 6, 7,
            5, 1, 2,  5, 2, 6,
            4, 0, 3,  4, 3, 7,
            3, 2, 6,  3, 6, 7,
            4, 5, 1,  4, 1, 0
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(0); // Unbind VAO
    }

    void drawCube() {
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    QVector3D calculateNormal(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3) {
        QVector3D edge1 = v2 - v1;
        QVector3D edge2 = v3 - v1;
        return QVector3D::crossProduct(edge1, edge2).normalized();
    }

    void drawPyramid() {
        const float size = 1.0f;
        const QVector3D apex(0.0f, size, 0.0f);

        QVector3D v1(-size, 0.0f, -size);
        QVector3D v2(size, 0.0f, -size);
        QVector3D v3(size, 0.0f, size);
        QVector3D v4(-size, 0.0f, size);

        glBegin(GL_TRIANGLES);

        QVector3D frontNormal = calculateNormal(v1, v2, apex);
        QVector3D rightNormal = calculateNormal(v2, v3, apex);
        QVector3D backNormal = calculateNormal(v3, v4, apex);
        QVector3D leftNormal = calculateNormal(v4, v1, apex);

        glVertexAttrib3f(1, frontNormal.x(), frontNormal.y(), frontNormal.z());
        glVertex3f(v1.x(), v1.y(), v1.z());
        glVertex3f(v2.x(), v2.y(), v2.z());
        glVertex3f(apex.x(), apex.y(), apex.z());

        glVertexAttrib3f(1, rightNormal.x(), rightNormal.y(), rightNormal.z());
        glVertex3f(v2.x(), v2.y(), v2.z());
        glVertex3f(v3.x(), v3.y(), v3.z());
        glVertex3f(apex.x(), apex.y(), apex.z());

        glVertexAttrib3f(1, backNormal.x(), backNormal.y(), backNormal.z());
        glVertex3f(v3.x(), v3.y(), v3.z());
        glVertex3f(v4.x(), v4.y(), v4.z());
        glVertex3f(apex.x(), apex.y(), apex.z());

        glVertexAttrib3f(1, leftNormal.x(), leftNormal.y(), leftNormal.z());
        glVertex3f(v4.x(), v4.y(), v4.z());
        glVertex3f(v1.x(), v1.y(), v1.z());
        glVertex3f(apex.x(), apex.y(), apex.z());

        QVector3D baseNormal(0.0f, -1.0f, 0.0f);
        glVertexAttrib3f(1, baseNormal.x(), baseNormal.y(), baseNormal.z());
        glVertex3f(v1.x(), v1.y(), v1.z());
        glVertex3f(v3.x(), v3.y(), v3.z());
        glVertex3f(v2.x(), v2.y(), v2.z());

        glVertexAttrib3f(1, baseNormal.x(), baseNormal.y(), baseNormal.z());
        glVertex3f(v1.x(), v1.y(), v1.z());
        glVertex3f(v4.x(), v4.y(), v4.z());
        glVertex3f(v3.x(), v3.y(), v3.z());

        glEnd();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Scene scene;
    scene.resize(800, 600);
    scene.show();

    return app.exec();
}

#include "main.moc"