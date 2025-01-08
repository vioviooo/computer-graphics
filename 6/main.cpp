#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QOpenGLFunctions_3_0>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QtMath>
#include <vector>

class Scene : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    Scene(QWidget *parent = nullptr) : QOpenGLWidget(parent), rotationAngle(0.0f), isPerspective(true) {
        setFocusPolicy(Qt::StrongFocus);
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Scene::onTimeout);
        timer->start(16);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        initShaders();
        setupPyramid();
        setupCube();
        setupSphere();
        qDebug() << "PyramidVAO: " << pyramidVAO;
        qDebug() << "CubeVAO: " << cubeVAO;
        qDebug() << "SphereVAO: " << sphereVAO;
        qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        projection.setToIdentity();

        if (isPerspective) {
            projection.perspective(45.0f, float(w) / float(h), 0.1f, 100.0f);
        } else {
            float size = 2.0f; // * size of the orthographic view
            projection.ortho(-size, size, -size, size, 0.1f, 100.0f);
        }
    }

    void updateProjection() {
        projection.setToIdentity();

        if (isPerspective) {
            projection.perspective(45.0f, width() / height(), 0.1f, 100.0f);
        } else {
            float size = 2.0f; // * size of the orthographic view
            projection.ortho(-size, size, -size, size, 0.1f, 100.0f);
        }
    }

   void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 view;
        QVector3D target = cameraPosition + QVector3D(0, 0, -1);
        view.lookAt(cameraPosition, target, QVector3D(0, 1, 0));

        view.rotate(rotationX, 1.0f, 0.0f, 0.0f);
        view.rotate(rotationY, 0.0f, 1.0f, 0.0f);

        shaderProgram.bind();
        shaderProgram.setUniformValue("pointLightPosition", pointLightPosition);
        shaderProgram.setUniformValue("pointLightColor", pointLightColor);
        shaderProgram.setUniformValue("dirLightDirection", dirLightDirection);
        shaderProgram.setUniformValue("dirLightColor", dirLightColor);
        shaderProgram.setUniformValue("cameraPos", cameraPosition);

        // * Draw floor (plane)
        QMatrix4x4 floorModel;
        floorModel.translate(0.0f, -0.5f, 0.0f);
        floorModel.scale(5.0f, 0.1f, 5.0f);
        QMatrix4x4 floorMVP = projection * view * floorModel;
        shaderProgram.setUniformValue("mvp", floorMVP);
        shaderProgram.setUniformValue("model", floorModel);
        shaderProgram.setUniformValue("isFloor", true);
        drawFloor();

        shaderProgram.setUniformValue("isFloor", false);

        // * Draw pyramid
        QMatrix4x4 pyramidModel;
        pyramidModel.translate(0.0f, -0.5f, -3.0f);
        QMatrix4x4 pyramidMVP = projection * view * pyramidModel;
        shaderProgram.setUniformValue("mvp", pyramidMVP);
        shaderProgram.setUniformValue("model", pyramidModel);
        drawPyramid();

        // * Draw cube
        QMatrix4x4 cubeModel;
        cubeModel.translate(-2.0f, 0.0f, -3.0f);
        QMatrix4x4 cubeMVP = projection * view * cubeModel;
        shaderProgram.setUniformValue("mvp", cubeMVP);
        shaderProgram.setUniformValue("model", cubeModel);
        drawCube();

        // * Draw sphere
        QMatrix4x4 sphereModel;
        sphereModel.translate(2.5f, 0.24f, -3.0f);
        float scaleFactor = 0.75f;
        sphereModel.scale(scaleFactor);
        QMatrix4x4 sphereMVP = projection * view * sphereModel;
        shaderProgram.setUniformValue("mvp", sphereMVP);
        shaderProgram.setUniformValue("model", sphereModel);
        drawSphere();

        GLint positionAttr = shaderProgram.attributeLocation("position");
        GLint normalAttr = shaderProgram.attributeLocation("normal");
        // qDebug() << "position attr:" << positionAttr << "normal attr:" << normalAttr;

        GLuint error = glGetError();
        if (error != GL_NO_ERROR) {
            qDebug() << "OpenGL error:" << error;
        }

        shaderProgram.release();
    }

    void keyPressEvent(QKeyEvent *event) override {
        qDebug() << "Key Pressed:" << event->key();
        float moveSpeed = 2.0f;
        switch (event->key()) {
        case Qt::Key_W:
            cameraPosition.setZ(cameraPosition.z() - moveSpeed);
            break;
        case Qt::Key_S:
            cameraPosition.setZ(cameraPosition.z() + moveSpeed);
            break;
        case Qt::Key_A:
            cameraPosition.setX(cameraPosition.x() - moveSpeed);
            break;
        case Qt::Key_D:
            cameraPosition.setX(cameraPosition.x() + moveSpeed);
            break;
        case Qt::Key_Up:
            cameraPosition.setY(cameraPosition.y() + moveSpeed);
            break;
        case Qt::Key_Down:
            cameraPosition.setY(cameraPosition.y() - moveSpeed);
            break;
        case Qt::Key_P:
            isPerspective = !isPerspective;
            updateProjection();
            update();
            break;
        }
        update();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            lastMousePosition = event->pos();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton) {
            QPoint delta = event->pos() - lastMousePosition;
            rotationX += delta.y() * 0.1f;
            rotationY += delta.x() * 0.1f;
            lastMousePosition = event->pos();
            update();
        }
    }

    void wheelEvent(QWheelEvent *event) override {
        float zoomSpeed = 0.1f;
        float delta = event->angleDelta().y() * zoomSpeed;
        cameraPosition.setZ(cameraPosition.z() + delta);
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
    QVector3D pointLightPosition = QVector3D(2.0f, 2.0f, -2.5f);
    QVector3D pointLightColor = QVector3D(0.2f, 1.0f, 0.2f);
    QVector3D dirLightDirection = QVector3D(0.0f, 0.0f, -1.0f);
    QVector3D dirLightColor = QVector3D(1.0f, 0.6f, 0.8f);
    QVector3D cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
    GLuint cubeVAO, cubeVBO;
    GLuint pyramidVAO, pyramidVBO;
    GLuint sphereVAO, sphereVBO;
    GLuint floorVAO, floorVBO, floorEBO;
    size_t spherePrimitivesCount;
    float rotationAngle;
    QPoint lastMousePosition;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    bool isPerspective;

    void initShaders() {
        const char *vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec3 normal;

            uniform mat4 mvp;
            uniform mat4 model;

            out vec3 fragPosition;
            out vec3 fragNormal;

            void main() {
                gl_Position = mvp * vec4(position, 1.0);
                fragPosition = vec3(model * vec4(position, 1.0));
                fragNormal = normalize(mat3(model) * normal);
            }
        )";
        
        const char *fragmentShaderSource = R"(
            #version 330 core

            in vec3 fragPosition;
            in vec3 fragNormal;

            uniform vec3 pointLightPosition;
            uniform vec3 pointLightColor;
            uniform vec3 dirLightDirection;
            uniform vec3 dirLightColor;
            uniform vec3 cameraPos;
            uniform bool isFloor;

            vec3 calcDirLight();
            vec3 calcPointLight();

            out vec4 fragColor;

            void main() {
                vec3 resultColor = vec3(0.0);
                if (isFloor) {
                    resultColor = vec3(0.5f, 0.5f, 0.5f);
                } else {
                    resultColor += calcPointLight();
                    resultColor += calcDirLight();
                }
                fragColor = vec4(resultColor, 1.0);
            }

            vec3 calcDirLight() {
                vec3 lightDir = normalize(-dirLightDirection);
                float diff = max(dot(fragNormal, lightDir), 0.0);

                vec3 viewDir = normalize(cameraPos - fragPosition);
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

                vec3 ambient = 0.05 * dirLightColor;
                vec3 diffuse = 0.5 * diff * dirLightColor;
                vec3 specular = 0.5 * spec * dirLightColor;

                return (ambient + diffuse + specular);
            }

            vec3 calcPointLight() {
                vec3 lightDir = normalize(pointLightPosition - fragPosition);
                float diff = max(dot(fragNormal, lightDir), 0.0);

                vec3 viewDir = normalize(cameraPos - fragPosition);
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

                float linearCoef = 0.15f;
                float quadraticCoef = 0.05f;
                float distance = length(pointLightPosition - fragPosition);
                float attenuation = 1.0 / (1.0 + linearCoef * distance + quadraticCoef * (distance * distance));

                vec3 ambient = attenuation * 0.05 * pointLightColor;
                vec3 diffuse = attenuation * 0.5 * diff * pointLightColor;
                vec3 specular = attenuation * 0.5 * spec * pointLightColor;

                return (ambient + diffuse + specular);
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

    void drawFloor() {
        GLfloat vertices[] = {
            -5.0f, 0.0f, -5.0f,
            5.0f, 0.0f, -5.0f,
            5.0f, 0.0f,  5.0f,
            -5.0f, 0.0f,  5.0f
        };

        GLuint indices[] = {0, 1, 2, 0, 2, 3};

        glGenVertexArrays(1, &floorVAO);
        glGenBuffers(1, &floorVBO);
        glGenBuffers(1, &floorEBO);

        glBindVertexArray(floorVAO);

        glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); // * Unbind VBO
        glBindVertexArray(0); // * Unbind VAO

        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setupSphere() {
        std::vector<GLfloat> vertices;

        const int slices = 100;
        const int stacks = 100;

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

                vertices.insert(vertices.end(), { v1.x(), v1.y(), v1.z() });
                vertices.insert(vertices.end(), { v1.x(), v1.y(), v1.z() });
                vertices.insert(vertices.end(), { v2.x(), v2.y(), v2.z() });
                vertices.insert(vertices.end(), { v2.x(), v2.y(), v2.z() });
                vertices.insert(vertices.end(), { v3.x(), v3.y(), v3.z() });
                vertices.insert(vertices.end(), { v3.x(), v3.y(), v3.z() });

                vertices.insert(vertices.end(), { v1.x(), v1.y(), v1.z() });
                vertices.insert(vertices.end(), { v1.x(), v1.y(), v1.z() });
                vertices.insert(vertices.end(), { v3.x(), v3.y(), v3.z() });
                vertices.insert(vertices.end(), { v3.x(), v3.y(), v3.z() });
                vertices.insert(vertices.end(), { v4.x(), v4.y(), v4.z() });
                vertices.insert(vertices.end(), { v4.x(), v4.y(), v4.z() });
            }
        }

        spherePrimitivesCount = vertices.size() / 6;

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

        GLuint positionLocation = shaderProgram.attributeLocation("position");
        GLuint normalLocation = shaderProgram.attributeLocation("normal");

        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(positionLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(normalLocation);

        glBindVertexArray(0); // * Unbind VAO
    }

    void drawSphere() {
        glBindVertexArray(sphereVAO);
        glDrawArrays(GL_TRIANGLES, 0, spherePrimitivesCount);
        glBindVertexArray(0);
    }

    QVector3D calculateNormal(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3) {
        QVector3D edge1 = v2 - v1;
        QVector3D edge2 = v3 - v1;
        return QVector3D::crossProduct(edge1, edge2).normalized();
    }

    void setupPyramid() {

        const float size = 1.0f;
        const QVector3D apex(0.0f, size, 0.0f);

        QVector3D v1(-size, 0.0f, -size);
        QVector3D v2(size, 0.0f, -size);
        QVector3D v3(size, 0.0f, size);
        QVector3D v4(-size, 0.0f, size);


        GLfloat vertices[] = {
            -0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,

             0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,
             0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,

            -0.5f,  0.0f, -0.5f,  0.0f,  0.5f, -1.0f,
             0.5f,  0.0f, -0.5f,  0.0f,  0.5f, -1.0f,
             0.0f,  1.0f,  0.0f,  0.0f,  0.5f, -1.0f,

             0.5f,  0.0f, -0.5f,  1.0f,  0.5f,  0.0f,
             0.5f,  0.0f,  0.5f,  1.0f,  0.5f,  0.0f,
             0.0f,  1.0f,  0.0f,  1.0f,  0.5f,  0.0f,

             0.5f,  0.0f,  0.5f,  0.0f,  0.5f,  1.0f,
            -0.5f,  0.0f,  0.5f,  0.0f,  0.5f,  1.0f,
             0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  1.0f,

            -0.5f,  0.0f,  0.5f, -1.0f,  0.5f,  0.0f,
            -0.5f,  0.0f, -0.5f, -1.0f,  0.5f,  0.0f,
             0.0f,  1.0f,  0.0f, -1.0f,  0.5f,  0.0f,
        };

        glGenVertexArrays(1, &pyramidVAO);
        glGenBuffers(1, &pyramidVBO);

        glBindVertexArray(pyramidVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        GLuint positionLocation = shaderProgram.attributeLocation("position");
        GLuint normalLocation = shaderProgram.attributeLocation("normal");

        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(positionLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(normalLocation);

        glBindVertexArray(0); // * Unbind VAO
    }

    void drawPyramid() {
        glBindVertexArray(pyramidVAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);
    }

    void setupCube() {
        GLfloat vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        GLuint positionLocation = shaderProgram.attributeLocation("position");
        GLuint normalLocation = shaderProgram.attributeLocation("normal");

        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(positionLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(normalLocation);

        if (!QOpenGLContext::currentContext()) {
            qDebug() << "No current OpenGL context!";
        }

        glBindVertexArray(0); // * Unbind VAO
    }

    void drawCube() {
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    Scene scene;
    scene.resize(800, 600);
    scene.show();

    return app.exec();
}

#include "main.moc"