#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <QTime>
#include <QPainter>
#include <QMatrix4x4>
#include <QColorDialog>
#include <QVector3D>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QtMath>
#include <vector>
#include <fstream>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <sstream>
#include <QPushButton>

class HUDWidget : public QWidget {
    Q_OBJECT

public:
    HUDWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents); // * надо чтобы виджет не блокировал события мыши
        setGeometry(0, 0, 200, 100); // * размер и положение HUD
        
        fpsTimer = new QTimer(this);
        connect(fpsTimer, &QTimer::timeout, this, &HUDWidget::updateFPS);
        fpsTimer->start(1000);

        fps = 0;
        objectCount = 0;
    }

    void setObjectCount(int count) {
        objectCount = count;
    }

    void setFPS(int fpsValue) {
        fps = fpsValue;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));

        painter.drawText(10, 20, QString("FPS: %1").arg(fps));
        painter.drawText(10, 40, QString("Objects: %1").arg(objectCount));
    }

private slots:
    void updateFPS() {
        fps = fpsCounter;
        fpsCounter = 0;
        update();
    }

public:
    void incrementFPSCounter() {
        fpsCounter++;
    }

private:
    int fps;
    int objectCount;
    int fpsCounter = 0;
    QTimer *fpsTimer;
};

class Scene : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

private:
    void reloadConfig() {
        QString configPath = "/Users/vioviooo/Desktop/computer-graphics/6/config.txt";
        objects.clear();
        loadObjectsFromFile(configPath);
        update();
    }

public:
    Scene(QWidget *parent = nullptr) : QOpenGLWidget(parent), rotationAngle(0.0f), isPerspective(true) {
        setFocusPolicy(Qt::StrongFocus);
    
        refreshButton = new QPushButton("Refresh", this);
        refreshButton->setGeometry(689, 10, 100, 30);
        connect(refreshButton, &QPushButton::clicked, this, &Scene::reloadConfig);

        colorPickerButton = new QPushButton("Change Color", this);
        colorPickerButton->setGeometry(689, 50, 100, 30);
        connect(colorPickerButton, &QPushButton::clicked, this, &Scene::openColorPicker);

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Scene::onTimeout);
        timer->start(16);

        QString configPath = "/Users/vioviooo/Desktop/computer-graphics/6/config.txt";
        loadObjectsFromFile(configPath);

        hudWidget = new HUDWidget(this);
        hudWidget->setParent(this);
        hudWidget->show();
    }

protected:
    struct RenderableObject {
        QString type;        // * object type: "Pyramid", "Cube", "Sphere"
        QMatrix4x4 model;    // * transformation matrix
        QVector3D position;  // * position in the scene
        float scale;         // * scale factor
        float rotation;      // * rotation angle (it won't rotate, it's for position rotation)
    };

    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        initShaders();
        setupPyramid();
        setupCube();
        setupSphere();

        QString configPath = "/Users/vioviooo/Desktop/computer-graphics/6/config.txt";
        loadObjectsFromFile(configPath);

        // qDebug() << "PyramidVAO: " << pyramidVAO;
        // qDebug() << "CubeVAO: " << cubeVAO;
        // qDebug() << "SphereVAO: " << sphereVAO;
        // qDebug() << "OpenGL Version:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));
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
        QMatrix4x4 cameraModel;

        cameraModel.rotate(rotationY, 0.0f, 1.0f, 0.0f);
        cameraModel.rotate(rotationX, 1.0f, 0.0f, 0.0f);

        QVector3D camera = cameraModel * cameraPosition;

        QVector3D target = QVector3D(0, 0, 0);
        view.lookAt(camera, target, QVector3D(0, 1, 0));

        shaderProgram.bind();
        shaderProgram.setUniformValue("pointLightPosition", pointLightPosition);
        shaderProgram.setUniformValue("pointLightColor", pointLightOn ? pointLightColor : QVector3D(0, 0, 0));
        shaderProgram.setUniformValue("dirLightDirection", dirLightDirection);
        shaderProgram.setUniformValue("dirLightColor", dirLightOn ? dirLightColor : QVector3D(0, 0, 0));
        shaderProgram.setUniformValue("spotLightPosition", spotLightPosition);
        shaderProgram.setUniformValue("spotLightDirection", spotLightOn ? spotLightDirection : QVector3D(0, 0, 0));
        shaderProgram.setUniformValue("spotLightColor", spotLightColor);
        shaderProgram.setUniformValue("spotLightInnerCutOffCos", (float) cos(spotLightInnerCutOff / 180 * 3.141592));
        shaderProgram.setUniformValue("spotLightOuterCutOffCos", (float) cos(spotLightOuterCutOff / 180 * 3.141592));
        shaderProgram.setUniformValue("cameraPos", camera);

        // * Draw floor (plane)
        QMatrix4x4 floorModel;
        floorModel.translate(0.0f, -0.5f, 0.0f);
        floorModel.scale(5.0f, 0.1f, 5.0f);
        QMatrix4x4 floorMVP = projection * view * floorModel;
        shaderProgram.setUniformValue("mvp", floorMVP);
        shaderProgram.setUniformValue("model", floorModel);
        shaderProgram.setUniformValue("objectColor", QVector3D(0.3f, 0.2f, 0.2f));
        shaderProgram.setUniformValue("isFloor", true);
        drawFloor();

        shaderProgram.setUniformValue("isFloor", false);

        // * bleugh dynamic objects
        for (const auto &object : objects) {
            QMatrix4x4 model = object.model;
            model.translate(object.position);
            model.scale(object.scale);
            model.rotate(object.rotation, QVector3D(0.0f, 1.0f, 0.0f));

            QMatrix4x4 mvp = projection * view * model;
            shaderProgram.setUniformValue("mvp", mvp);
            shaderProgram.setUniformValue("model", model);
            shaderProgram.setUniformValue("objectColor", QVector3D(1.0f, 1.0f, 1.0f));

            if (object.type == "Pyramid") {
                drawPyramid();
            } else if (object.type == "Cube") {
                drawCube();
            } else if (object.type == "Sphere") {
                drawSphere();
            }
        }

        GLuint error = glGetError();
        if (error != GL_NO_ERROR) {
            qDebug() << "OpenGL error:" << error;
        }

        shaderProgram.release();

        if (pointLightOn) {
            pointLightShaderProgram.bind();

            QMatrix4x4 model;
            model.translate(pointLightPosition);
            model.scale(0.1f);

            QMatrix4x4 mvp = projection * view * model;
            pointLightShaderProgram.setUniformValue("mvp", mvp);
            pointLightShaderProgram.setUniformValue("model", model);
            pointLightShaderProgram.setUniformValue("pointLightColor", pointLightColor);

            drawSphere();

            pointLightShaderProgram.release();
        }

        // * HUD widget
        hudWidget->setObjectCount(objects.size());
        hudWidget->incrementFPSCounter();  
    }

    void keyPressEvent(QKeyEvent *event) override {
        qDebug() << "Key Pressed:" << event->key();
        float moveSpeed = 0.5f;
        switch (event->key()) {
        case Qt::Key_W:
            cameraPosition.setZ(cameraPosition.z() - moveSpeed);
            if (cameraPosition.z() < 0.5) {
                cameraPosition.setZ(0.5);
            }
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
            break;
        case Qt::Key_1:
            pointLightOn = !pointLightOn;
            break;
        case Qt::Key_2:
            dirLightOn = !dirLightOn;
            break;
        case Qt::Key_3:
            spotLightOn = !spotLightOn;
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

            if (rotationX > 88) {
                rotationX = 88;
            }
            if (rotationX < -88) {
                rotationX = -88;
            }

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

    void openColorPicker() {
        QColor selectedColor = QColorDialog::getColor(Qt::white, this, "Select Light Color");
        if (selectedColor.isValid()) {
            pointLightColor = QVector3D(
                selectedColor.redF(),
                selectedColor.greenF(),
                selectedColor.blueF()
            );
            update();
        }
    }

private:
    QTimer *timer;
    QMatrix4x4 projection;
    QOpenGLShaderProgram shaderProgram;
    QOpenGLShaderProgram pointLightShaderProgram;
    QVector3D pointLightPosition = QVector3D(2.0f, 2.0f, -2.5f);
    QVector3D pointLightColor = QVector3D(0.2f, 1.0f, 0.2f);
    QVector3D dirLightDirection = QVector3D(0.0f, -1.0f, 1.0f);
    QVector3D dirLightColor = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D spotLightPosition = QVector3D(0.0f, 10.0f, 0.0f);
    QVector3D spotLightDirection = QVector3D(0.0f, -1.0f, 0.0f);
    QVector3D spotLightColor = QVector3D(1.0f, 1.0f, 1.0f);
    float spotLightInnerCutOff = 20.0f;
    float spotLightOuterCutOff = 30.0f;
    bool dirLightOn = true;
    bool pointLightOn = true;
    bool spotLightOn = true;
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
    std::vector<RenderableObject> objects;
    HUDWidget* hudWidget;
    QPushButton* refreshButton;
    QPushButton* colorPickerButton;

    void initShaders() {
        const char *vertexShaderSource = R"(
            #version 330 core

            #undef mediump
            precision mediump float;

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

            #undef mediump
            precision mediump float;

            in vec3 fragPosition;
            in vec3 fragNormal;

            uniform vec3 objectColor;
            uniform vec3 pointLightPosition;
            uniform vec3 pointLightColor;
            uniform vec3 dirLightDirection;
            uniform vec3 dirLightColor;
            uniform vec3 spotLightPosition;
            uniform vec3 spotLightColor;
            uniform vec3 spotLightDirection;
            uniform float spotLightInnerCutOffCos;
            uniform float spotLightOuterCutOffCos;
            uniform vec3 cameraPos;
            uniform bool isFloor;

            vec3 calcDirLight(float ambiCoef, float diffCoef, float specCoef);
            vec3 calcPointLight(float ambiCoef, float diffCoef, float specCoef);
            vec3 calcSpotLight(float ambiCoef, float diffCoef, float specCoef);

            out vec4 fragColor;

            void main() {
                vec3 resultColor = vec3(0.0);
                if (isFloor) {
                    // resultColor = vec3(0.5f, 0.5f, 0.5f);
                    resultColor += calcPointLight(0.15, 0.7, 0.3);
                    resultColor += calcDirLight(0.15, 0.7, 0.3);
                    resultColor += calcSpotLight(0.15, 0.7, 0.3);
                } else {
                    resultColor += calcPointLight(0.05, 0.5, 0.5);
                    resultColor += calcDirLight(0.05, 0.5, 0.5);
                    resultColor += calcSpotLight(0.05, 0.5, 0.5);
                }
                fragColor = vec4(resultColor, 1.0);
            }

            vec3 calcDirLight(float ambiCoef, float diffCoef, float specCoef) {
                vec3 lightDir = normalize(-dirLightDirection);
                float diff = max(dot(fragNormal, lightDir), 0.0);

                vec3 viewDir = normalize(cameraPos - fragPosition);
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);

                vec3 ambient  = ambiCoef * dirLightColor;
                vec3 diffuse  = diffCoef * diff * dirLightColor;
                vec3 specular = specCoef * spec * dirLightColor;

                return (ambient + diffuse + specular) * objectColor;
            }

            vec3 calcPointLight(float ambiCoef, float diffCoef, float specCoef) {
                vec3 lightDir = normalize(pointLightPosition - fragPosition);
                float diff = max(dot(fragNormal, lightDir), 0.0);

                vec3 viewDir = normalize(cameraPos - fragPosition);
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);

                float linearCoef = 0.15f;
                float quadraticCoef = 0.05f;
                float distance = length(pointLightPosition - fragPosition);
                float attenuation = 1.0 / (1.0 + linearCoef * distance + quadraticCoef * (distance * distance));

                vec3 ambient  = attenuation * ambiCoef * pointLightColor;
                vec3 diffuse  = attenuation * diffCoef * diff * pointLightColor;
                vec3 specular = attenuation * specCoef * spec * pointLightColor;

                return (ambient + diffuse + specular) * objectColor;
            }

            vec3 calcSpotLight(float ambiCoef, float diffCoef, float specCoef) {
                vec3 lightDir = normalize(spotLightPosition - fragPosition);
                float diff = max(dot(fragNormal, lightDir), 0.0);

                vec3 viewDir = normalize(cameraPos - fragPosition);
                vec3 reflectDir = reflect(-lightDir, fragNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);

                float theta = dot(lightDir, normalize(-spotLightDirection));
                float epsilon = (spotLightInnerCutOffCos - spotLightOuterCutOffCos);
                float intensity = clamp((theta - spotLightOuterCutOffCos) / epsilon, 0.0, 1.0);

                float linearCoef = 0.045f;
                float quadraticCoef = 0.0075f;
                float distance = length(spotLightPosition - fragPosition);
                float attenuation = 1.0 / (1.0 + linearCoef * distance + quadraticCoef * (distance * distance));

                vec3 ambient  = attenuation * ambiCoef * spotLightColor;
                vec3 diffuse  = attenuation * diffCoef * diff * spotLightColor * intensity;
                vec3 specular = attenuation * specCoef * spec * spotLightColor * intensity;

                return (ambient + diffuse + specular) * objectColor;
            }
        )";

        const char *fragmentPointLightShaderSource = R"(
            #version 330 core

            #undef mediump
            precision mediump float;

            in vec3 fragPosition;
            in vec3 fragNormal;

            uniform vec3 pointLightColor;

            out vec4 fragColor;

            void main() {
                fragColor = vec4(pointLightColor, 1.0);
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
            return;
        }
        shaderProgram.release();

        if (!pointLightShaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
            qDebug() << "Error compiling vertex shader:" << shaderProgram.log();
            return;
        }

        if (!pointLightShaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentPointLightShaderSource)) {
            qDebug() << "Error compiling fragment shader:" << shaderProgram.log();
            return;
        }

        if (!pointLightShaderProgram.link()) {
            qDebug() << "Error linking shader program:" << shaderProgram.log();
            return;
        }

        if (!pointLightShaderProgram.bind()) {
            qDebug() << "Error binding shader program:" << shaderProgram.log();
            return;
        }
        pointLightShaderProgram.release();
    }
    
    void drawFloor() {
        GLfloat vertices[] = {
            -5.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f,
             5.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f,
             5.0f, 0.0f,  5.0f, 0.0f, 1.0f, 0.0f,
            -5.0f, 0.0f,  5.0f, 0.0f, 1.0f, 0.0f,
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

        GLuint positionLocation = shaderProgram.attributeLocation("position");
        GLuint normalLocation = shaderProgram.attributeLocation("normal");

        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(positionLocation);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(normalLocation);

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

    void loadObjectsFromFile(const QString &filePath) {
        std::ifstream file(filePath.toStdString());
        if (!file.is_open()) {
            qDebug() << "YO Failed to open configuration file:" << filePath;
            return;
        }

        objects.clear(); // * clear existing objects

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            float x, y, z, scale, rotation;

            if (!(iss >> type >> x >> y >> z >> scale >> rotation)) {
                qDebug() << "Invalid line in config file:" << QString::fromStdString(line);
                continue;
            }

            objects.push_back({
                QString::fromStdString(type),
                QMatrix4x4(),
                QVector3D(x, y, z),
                scale,
                rotation
            });
        }

        file.close();
        qDebug() << "Loaded objects from file:" << filePath;
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