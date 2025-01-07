#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_0>
#include <QTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QtMath>

#include <vector>

class Scene : public QOpenGLWidget, protected QOpenGLFunctions_3_0 {
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
        // initShaders();
        // setupSphere();
        // setupCube();
        setupPyramid();
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
        shaderProgram.setUniformValue("pointLightPosition", pointLightPosition);
        shaderProgram.setUniformValue("pointLightColor", pointLightColor);
        shaderProgram.setUniformValue("dirLightDirection", dirLightDirection);
        shaderProgram.setUniformValue("dirLightColor", dirLightColor);
        shaderProgram.setUniformValue("cameraPos", cameraPosition);

        // Draw cube
        // QMatrix4x4 cubeModel;
        // cubeModel.translate(-2.0f, 0.0f, -3.0f);
        // cubeModel.rotate(rotationAngle, 0.5f, 1.0f, 0.0f);
        // QMatrix4x4 cubeMVP = projection * view * cubeModel;
        // shaderProgram.setUniformValue("mvp", cubeMVP);
        // shaderProgram.setUniformValue("model", cubeModel);
        // drawCube();

        // // Draw sphere
        // QMatrix4x4 sphereModel;
        // sphereModel.translate(2.5f, 0.0f, -3.0f);
        // sphereModel.rotate(rotationAngle, 0.5f, 1.0f, 0.0f);
        // QMatrix4x4 sphereMVP = projection * view * sphereModel;
        // shaderProgram.setUniformValue("mvp", sphereMVP);
        // shaderProgram.setUniformValue("model", sphereModel);
        // drawSphere();

        // // Draw pyramid
        // QMatrix4x4 pyramidModel;
        // pyramidModel.translate(0.0f, -0.5f, -3.0f);
        // pyramidModel.rotate(30.0f, 0.5f, 0.0f, 1.0f);
        // pyramidModel.rotate(rotationAngle, 0.0f, 1.0f, 0.0f);
        // QMatrix4x4 pyramidMVP = projection * view * pyramidModel;
        // shaderProgram.setUniformValue("mvp", pyramidMVP);
        // shaderProgram.setUniformValue("model", pyramidModel);
        // drawPyramid();

        // shaderProgram.bind();

        // GLuint error = glGetError();
        // if (error != GL_NO_ERROR) {
        //     qDebug() << "OpenGL error:" << error;
        // }

        // shaderProgram.release();
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
    QVector3D pointLightPosition = QVector3D(2.0f, 2.0f, -2.5f);
    QVector3D pointLightColor = QVector3D(0.2f, 1.0f, 0.2f);
    QVector3D dirLightDirection = QVector3D(0.0f, 0.0f, -1.0f);
    QVector3D dirLightColor = QVector3D(1.0f, 0.2f, 0.2f);
    QVector3D cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
    GLuint cubeVAO, cubeVBO;
    GLuint sphereVAO, sphereVBO;
    GLuint pyramidVAO, pyramidVBO;
    size_t spherePrimitivesCount;

    float rotationAngle;

    void initShaders() {
        const char *vertexShaderSource = R"(
            #version 120
            attribute vec3 position;
            attribute vec3 normal;

            uniform mat4 mvp;
            uniform mat4 model;

            varying vec3 fragPosition;
            varying vec3 fragNormal;

            void main() {
                gl_Position = mvp * vec4(position, 1.0);
                fragPosition = vec3(model * vec4(position, 1.0));
                fragNormal = normalize(mat3(model) * normal);
            }
        )";

        const char *fragmentShaderSource = R"(
            #version 120

            varying vec3 fragPosition;
            varying vec3 fragNormal;

            uniform vec3 pointLightPosition;
            uniform vec3 pointLightColor;
            uniform vec3 dirLightDirection;
            uniform vec3 dirLightColor;
            uniform vec3 cameraPos;

            vec3 calcDirLight();
            vec3 calcPointLight();

            void main() {
                vec3 resultColor = vec3(0.0);
                resultColor += calcPointLight();
                resultColor += calcDirLight();
                gl_FragColor = vec4(resultColor, 1.0);
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

    void setupSphere() {
        std::vector<GLfloat> vertices;

        const int slices = 40;
        const int stacks = 40;

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

        glBindVertexArray(0); // Unbind VAO
    }

    void drawSphere() {
        glBindVertexArray(sphereVAO);
        glDrawArrays(GL_TRIANGLES, 0, spherePrimitivesCount);
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

        // GLuint EBO;
        // glGenBuffers(1, &EBO);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(0); // Unbind VAO
    }

    void drawCube() {
        glBindVertexArray(cubeVAO);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
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
            // base
            -0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,

             0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,
             0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f,

            // lateral
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

        glBindVertexArray(0); // Unbind VAO
    }

    void drawPyramid() {
        glBindVertexArray(pyramidVAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);
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
