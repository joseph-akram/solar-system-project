#include <GL/freeglut.h> // Changed from <GLUT/glut.h> to use FreeGLUT
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Star {
    float x, y, z;                       
    float brightness;                            
    float size;                            
    int colorType;                       
};

struct Planet {
    std::string name;
    float radius;
    float distance;
    float speed;
    float tilt;
    float spinSpeed;
    bool hasRings;
    bool hasMoon;
    float r, g, b;
    float specularR, specularG, specularB;
    float shininess;
    float atmosphereDensity;
    GLuint textureId = 0;
};

float globalTime = 0.0f;
float twinkleTime = 0.0f;

float cameraAngleX = 25.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 45.0f;
float cameraPanX = 0.0f;
float cameraPanY = 0.0f;

bool animationRunning = true;
float animationSpeed = 1.0f;
bool showLabels = true;

bool mouseLeftDown = false;
bool mouseRightDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

std::vector<Star> stars;
std::vector<Planet> planets;

GLUquadric* quad = nullptr;
GLuint sunTex = 0;
GLuint moonTex = 0;
GLuint saturnRingsTex = 0;

GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat lightAmbient[] = { 0.20f, 0.20f, 0.25f, 1.0f };
GLfloat lightDiffuse[] = { 1.00f, 0.95f, 0.85f, 1.0f };
GLfloat lightSpecular[] = { 1.00f, 1.00f, 1.00f, 1.0f };
GLfloat lightBackPos[] = { -20.0f, 10.0f, -20.0f, 1.0f };
GLfloat lightBackDiffuse[] = { 0.30f, 0.30f, 0.50f, 1.0f };

float clampf(float v, float mn, float mx) {
    return std::max(mn, std::min(v, mx));
}

float randomRange(float mn, float mx) {
    return mn + (rand() / (float)RAND_MAX) * (mx - mn);
}

void drawString(float x, float y, void* font, const std::string& s) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void drawString3D(float x, float y, float z, void* font, const std::string& s) {
    glRasterPos3f(x, y, z);
    for (char c : s) glutBitmapCharacter(font, c);
}

GLuint createFallbackTexture(const std::string& name, bool alpha = false) {
    const int w = 512;
    const int h = 512;
    GLuint tex = 0;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (!alpha) {
        std::vector<unsigned char> data(w * h * 3);
        auto put = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b) {
            int i = (y * w + x) * 3;
            data[i + 0] = r;
            data[i + 1] = g;
            data[i + 2] = b;
            };     

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float u = x / float(w - 1);
                float v = y / float(h - 1);
                float dx = u - 0.5f;
                float dy = v - 0.5f;
                float dist = std::sqrt(dx * dx + dy * dy) / 0.7071f;
                dist = clampf(dist, 0.0f, 1.0f);

                unsigned char r = 0, g = 0, b = 0;

                if (name == "Sun") {
                    float core = 1.0f - dist;
                    float wave = 0.5f + 0.5f * std::sin((u * 20.0f + v * 14.0f) * 6.28318f);
                    float t = clampf(core * 0.75f + wave * 0.25f, 0.0f, 1.0f);
                    r = (unsigned char)(255 * (1.00f * t + 0.12f * (1.0f - t)));
                    g = (unsigned char)(255 * (0.75f * t + 0.10f * (1.0f - t)));
                    b = (unsigned char)(255 * (0.20f * t + 0.04f * (1.0f - t)));
                }
                else if (name == "Earth") {
                    float mix = 0.5f + 0.5f * std::sin(u * 20.0f) * std::cos(v * 14.0f);
                    float rr = 0.15f * (1.0f - mix) + 0.12f * mix;
                    float gg = 0.30f * (1.0f - mix) + 0.55f * mix;
                    float bb = 0.55f * (1.0f - mix) + 0.95f * mix;
                    r = (unsigned char)(255 * (rr * (1.0f - dist) + 0.10f * dist));
                    g = (unsigned char)(255 * (gg * (1.0f - dist) + 0.10f * dist));
                    b = (unsigned char)(255 * (bb * (1.0f - dist) + 0.12f * dist));
                }
                else if (name == "Moon") {
                    float c = 0.5f + 0.5f * std::sin(u * 24.0f + v * 19.0f);
                    float t = 0.8f * (1.0f - dist) + 0.2f * c;
                    r = g = b = (unsigned char)(255 * (0.75f * t + 0.18f * (1.0f - t)));
                }
                else {
                    float t = 1.0f - dist;
                    r = g = b = (unsigned char)(255 * t);
                }

                put(x, y, r, g, b);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    }
    else {
        std::vector<unsigned char> data(w * h * 4);
        auto put = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
            int i = (y * w + x) * 4;
            data[i + 0] = r;
            data[i + 1] = g;
            data[i + 2] = b;
            data[i + 3] = a;
            };

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float u = x / float(w - 1);
                float v = y / float(h - 1);
                float dx = u - 0.5f;
                float dy = v - 0.5f;
                float dist = std::sqrt(dx * dx + dy * dy);
                unsigned char r = 0, g = 0, b = 0, a = 0;

                if (name == "SaturnRings" && dist > 0.22f && dist < 0.48f) {
                    float band = 0.5f + 0.5f * std::sin(dist * 140.0f + u * 12.0f);
                    r = (unsigned char)(255 * (0.82f * band));
                    g = (unsigned char)(255 * (0.74f * band));
                    b = (unsigned char)(255 * (0.62f * band));
                    a = (unsigned char)(255 * (0.8f * band));
                }

                put(x, y, r, g, b, a);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint loadTextureOrFallback(const std::string& path, const std::string& fallbackName, bool alpha = false) {
    int w = 0, h = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, alpha ? STBI_rgb_alpha : STBI_rgb);
    if (!data) {
        std::cout << "Missing: " << path << " -> using fallback texture for " << fallbackName << "\n";
        return createFallbackTexture(fallbackName, alpha);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format = alpha ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void initStars() {
    srand(12345);
    stars.clear();

    for (int i = 0; i < 4500; ++i) {
        Star s;
        float radius = randomRange(100.0f, 180.0f);
        float theta = randomRange(0.0f, 2.0f * (float)M_PI);
        float phi = randomRange(0.0f, 2.0f * (float)M_PI);
        s.x = radius * std::sin(theta) * std::cos(phi);
        s.y = radius * std::sin(theta) * std::sin(phi) * 0.5f;
        s.z = radius * std::cos(theta);
        s.size = randomRange(1.0f, 3.0f);
        s.brightness = randomRange(0.4f, 1.0f);
        s.colorType = rand() % 100;
        stars.push_back(s);
    }
}

void initPlanets() {
    planets.clear();

    planets.push_back({ "Mercury", 0.42f, 5.0f, 4.50f, 0.0f, 8.0f, false, false, 0.65f, 0.55f, 0.48f, 0.30f, 0.30f, 0.30f, 30.0f, 0.0f });
    planets.push_back({ "Venus",   0.52f, 7.0f, 3.20f, 3.0f,  6.5f, false, false, 0.85f, 0.70f, 0.50f, 0.60f, 0.50f, 0.30f, 40.0f, 0.4f });
    planets.push_back({ "Earth",   0.56f, 9.0f, 2.80f, 23.5f, 8.0f, false, true,  0.20f, 0.50f, 0.90f, 0.30f, 0.50f, 0.80f, 60.0f, 0.3f });
    planets.push_back({ "Mars",    0.51f, 11.0f, 2.20f, 25.0f, 7.0f, false, false, 0.80f, 0.40f, 0.25f, 0.50f, 0.30f, 0.20f, 35.0f, 0.1f });
    planets.push_back({ "Jupiter", 1.15f, 14.0f, 1.30f, 3.0f,  5.5f, false, false, 0.70f, 0.60f, 0.45f, 0.80f, 0.70f, 0.50f, 70.0f, 0.0f });
    planets.push_back({ "Saturn",  0.98f, 17.0f, 1.00f, 27.0f, 5.0f, true,  false, 0.78f, 0.72f, 0.55f, 0.70f, 0.60f, 0.50f, 65.0f, 0.0f });
    planets.push_back({ "Uranus",  0.82f, 20.0f, 0.70f, 98.0f, 4.5f, false, false, 0.55f, 0.80f, 0.85f, 0.50f, 0.70f, 0.80f, 50.0f, 0.2f });
    planets.push_back({ "Neptune", 0.81f, 23.0f, 0.50f, 28.0f, 4.0f, false, false, 0.25f, 0.45f, 0.85f, 0.30f, 0.50f, 0.80f, 55.0f, 0.15f });
}

void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    glLightfv(GL_LIGHT1, GL_POSITION, lightBackPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightBackDiffuse);

    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);

    initStars();
}

void loadAllTextures() {
    sunTex = loadTextureOrFallback("textures/sun.jpg", "Sun");
    moonTex = loadTextureOrFallback("textures/moon.jpg", "Moon");
    saturnRingsTex = loadTextureOrFallback("textures/saturn_rings.png", "SaturnRings", true);

    for (auto& p : planets) {
        if (p.name == "Mercury") p.textureId = loadTextureOrFallback("textures/mercury.jpg", "Mercury");
        if (p.name == "Venus")   p.textureId = loadTextureOrFallback("textures/venus.jpg", "Venus");
        if (p.name == "Earth")   p.textureId = loadTextureOrFallback("textures/earth.jpg", "Earth");
        if (p.name == "Mars")    p.textureId = loadTextureOrFallback("textures/mars.jpg", "Mars");
        if (p.name == "Jupiter") p.textureId = loadTextureOrFallback("textures/jupiter.jpg", "Jupiter");
        if (p.name == "Saturn")  p.textureId = loadTextureOrFallback("textures/saturn.jpg", "Saturn");
        if (p.name == "Uranus")  p.textureId = loadTextureOrFallback("textures/uranus.jpg", "Uranus");
        if (p.name == "Neptune") p.textureId = loadTextureOrFallback("textures/neptune.jpg", "Neptune");
    }
}

void cleanup() {
    if (quad) {
        gluDeleteQuadric(quad);
        quad = nullptr;
    }

    if (sunTex) glDeleteTextures(1, &sunTex);
    if (moonTex) glDeleteTextures(1, &moonTex);
    if (saturnRingsTex) glDeleteTextures(1, &saturnRingsTex);

    for (auto& p : planets) {
        if (p.textureId) glDeleteTextures(1, &p.textureId);
    }
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (const auto& s : stars) {
        float twinkle = 0.7f + 0.3f * std::sin(twinkleTime * s.brightness * 3.0f + s.x);
        float b = s.brightness * twinkle;

        if (s.colorType > 97) {
            glColor3f(b * 1.0f, b * 0.6f, b * 0.4f);
        }
        else if (s.colorType > 94) {
            glColor3f(b * 0.5f, b * 0.7f, b * 1.0f);
        }
        else {
            glColor3f(b, b, b);
        }

        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawTexturedSphere(GLUquadric* q, float radius, GLuint tex, bool emissive = false) {
    glPushMatrix();
    glScalef(radius, radius, radius);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (emissive) {
        GLfloat emission[] = { 1.0f, 0.8f, 0.3f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emission);
    }

    gluSphere(q, 1.0, 96, 96);

    if (emissive) {
        GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void drawOrbit(float radius, float tiltAngle) {
    (void)tiltAngle;

    glDisable(GL_LIGHTING);
    glColor4f(0.55f, 0.55f, 0.75f, 0.35f);
    glLineWidth(1.0f);

    // Flat, clean orbit circle in the XZ plane
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 2) {
        float a = i * (float)M_PI / 180.0f;
        glVertex3f(radius * std::cos(a), 0.0f, radius * std::sin(a));
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void drawPlanetLabel(const std::string& name, float x, float y, float z) {
    if (!showLabels) return;
    glDisable(GL_LIGHTING);
    glColor3f(0.95f, 0.95f, 0.70f);
    drawString3D(x - name.length() * 0.12f, y + 0.8f, z, GLUT_BITMAP_HELVETICA_12, name);
    glEnable(GL_LIGHTING);
}

void drawMoon(float earthX, float earthZ, float angle) {
    float moonOrbitRadius = 1.35f;
    float moonX = earthX + moonOrbitRadius * std::cos(angle);
    float moonZ = earthZ + moonOrbitRadius * std::sin(angle);

    glDisable(GL_LIGHTING);
    glColor4f(0.5f, 0.5f, 0.6f, 0.25f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 10) {
        float a = i * (float)M_PI / 180.0f;
        glVertex3f(earthX + moonOrbitRadius * std::cos(a), 0.0f, earthZ + moonOrbitRadius * std::sin(a));
    }
    glEnd();
    glEnable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(moonX, 0.12f, moonZ);
    drawTexturedSphere(quad, 0.18f, moonTex);
    glPopMatrix();
}

void drawSaturnRings(float planetRadius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(10.0f, 1.0f, 0.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, saturnRingsTex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    float innerR = planetRadius * 1.18f;
    float outerR = planetRadius * 2.05f;

    glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 360; i += 4) {
        float a = i * (float)M_PI / 180.0f;
        float ca = std::cos(a);
        float sa = std::sin(a);

        glTexCoord2f((float)i / 360.0f, 0.0f);
        glVertex3f(innerR * ca, 0.0f, innerR * sa);

        glTexCoord2f((float)i / 360.0f, 1.0f);
        glVertex3f(outerR * ca, 0.0f, outerR * sa);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawSun() {
    glPushMatrix();
    drawTexturedSphere(quad, 3.2f, sunTex, true);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (int i = 0; i < 5; ++i) {
        float scale = 3.4f + i * 0.35f;
        float alpha = 0.12f - i * 0.015f;
        glColor4f(1.0f, 0.55f, 0.15f, alpha);
        glPushMatrix();
        glScalef(scale, scale, scale);
        gluSphere(quad, 1.0, 28, 28);
        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawPlanet(const Planet& p, float orbitAngle, float moonAng) {
    float x = p.distance * std::cos(orbitAngle);
    float z = p.distance * std::sin(orbitAngle);

    drawOrbit(p.distance, p.tilt * 0.25f);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(p.tilt, 1.0f, 0.0f, 0.0f);
    glRotatef(globalTime * p.spinSpeed * 15.0f, 0.0f, 1.0f, 0.0f);

    GLfloat matAmbient[] = { p.r * 0.5f, p.g * 0.5f, p.b * 0.5f, 1.0f };
    GLfloat matDiffuse[] = { p.r, p.g, p.b, 1.0f };
    GLfloat matSpecular[] = { p.specularR, p.specularG, p.specularB, 1.0f };
    GLfloat matShininess[] = { p.shininess };

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    drawTexturedSphere(quad, p.radius, p.textureId);

    if (p.atmosphereDensity > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING);
        glColor4f(p.r, p.g, p.b, p.atmosphereDensity * 0.18f);
        glPushMatrix();
        glScalef(p.radius * 1.07f, p.radius * 1.07f, p.radius * 1.07f);
        gluSphere(quad, 1.0, 48, 48);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glDisable(GL_BLEND);
    }

    if (p.hasRings) {
        drawSaturnRings(p.radius);
    }

    glPopMatrix();

    drawPlanetLabel(p.name, x, p.radius, z);

    if (p.hasMoon && p.name == "Earth") {
        drawMoon(x, z, moonAng);
    }
}

void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawString(10, h - 25, GLUT_BITMAP_HELVETICA_12, "=== 3D SOLAR SYSTEM TEXTURED ===");
    drawString(10, h - 45, GLUT_BITMAP_HELVETICA_10, "Left Drag: Rotate | Right Drag: Pan");
    drawString(10, h - 60, GLUT_BITMAP_HELVETICA_10, "W/S: Zoom | Arrows: Pan | SPACE: Pause | +/-: Speed | L: Labels | R: Reset");
    drawString(10, h - 80, GLUT_BITMAP_HELVETICA_10, std::string("Speed: x") + std::to_string(animationSpeed));
    drawString(10, h - 95, GLUT_BITMAP_HELVETICA_10, animationRunning ? "Status: RUNNING" : "Status: PAUSED");
    drawString(10, h - 110, GLUT_BITMAP_HELVETICA_10, showLabels ? "Labels: ON" : "Labels: OFF");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    twinkleTime += 0.02f;

    float camX = cameraDistance * std::sin(cameraAngleY * (float)M_PI / 180.0f) * std::cos(cameraAngleX * (float)M_PI / 180.0f) + cameraPanX;
    float camY = cameraDistance * std::sin(cameraAngleX * (float)M_PI / 180.0f) + cameraPanY;
    float camZ = cameraDistance * std::cos(cameraAngleY * (float)M_PI / 180.0f) * std::cos(cameraAngleX * (float)M_PI / 180.0f);

    gluLookAt(camX, camY + 5.0f, camZ,
        cameraPanX, cameraPanY, 0.0f,
        0.0f, 1.0f, 0.0f);

    drawStars();
    drawSun();

    for (size_t i = 0; i < planets.size(); ++i) {
        float orbitAngle = globalTime * planets[i].speed * animationSpeed;
        float moonAng = (planets[i].name == "Earth") ? globalTime * 8.0f * animationSpeed : 0.0f;
        drawPlanet(planets[i], orbitAngle, moonAng);
    }

    drawHUD();
    glutSwapBuffers();
}

void update(int) {
    if (animationRunning) {
        globalTime += 0.02f;
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 250.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
    case 27:
        cleanup();
        std::exit(0);
        break;
    case ' ': animationRunning = !animationRunning; break;
    case 'l': case 'L': showLabels = !showLabels; break;
    case 'w': case 'W': cameraDistance = std::max(10.0f, cameraDistance - 2.0f); break;
    case 's': case 'S': cameraDistance = std::min(90.0f, cameraDistance + 2.0f); break;
    case 'r': case 'R':
        cameraAngleX = 25.0f;
        cameraAngleY = 0.0f;
        cameraDistance = 45.0f;
        cameraPanX = 0.0f;
        cameraPanY = 0.0f;
        animationSpeed = 1.0f;
        break;
    case '+': case '=': animationSpeed = std::min(4.0f, animationSpeed + 0.2f); break;
    case '-': case '_': animationSpeed = std::max(0.2f, animationSpeed - 0.2f); break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    switch (key) {
    case GLUT_KEY_LEFT:      cameraPanX -= 2.0f; break;
    case GLUT_KEY_RIGHT:     cameraPanX += 2.0f; break;
    case GLUT_KEY_UP:        cameraPanY += 1.5f; break;
    case GLUT_KEY_DOWN:      cameraPanY -= 1.5f; break;
    case GLUT_KEY_PAGE_UP:   cameraDistance = std::max(8.0f, cameraDistance - 2.0f); break;
    case GLUT_KEY_PAGE_DOWN: cameraDistance = std::min(90.0f, cameraDistance + 2.0f); break;
    case GLUT_KEY_HOME:
        cameraAngleX = 25.0f;
        cameraAngleY = 0.0f;
        cameraDistance = 45.0f;
        cameraPanX = 0.0f;
        cameraPanY = 0.0f;
        break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseLeftDown = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        mouseRightDown = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }
}

void mouseMotion(int x, int y) {
    if (mouseLeftDown) {
        cameraAngleY += (x - lastMouseX) * 0.5f;
        cameraAngleX += (y - lastMouseY) * 0.3f;
        cameraAngleX = clampf(cameraAngleX, -80.0f, 80.0f);
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }

    if (mouseRightDown) {
        cameraPanX += (x - lastMouseX) * 0.1f;
        cameraPanY -= (y - lastMouseY) * 0.1f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D Solar System - Textured");

    initPlanets();
    initOpenGL();
    loadAllTextures();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, update, 0);

    atexit(cleanup);

    glutMainLoop();
    return 0;
}