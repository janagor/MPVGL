#include <glm/gtc/matrix_transform.hpp>

#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Geometry/Shape.hpp"

int main() {
    mpvgl::Scene scene{};

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::literal::Red),
              glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)));

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::literal::Green),
              glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

    scene.add(mpvgl::Shape<mpvgl::Quad>::generate(mpvgl::Color::literal::Blue),
              glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 1.0f, 0.0f)));

    mpvgl::RenderWindow window(1280, 720, "Mój Super Silnik", scene);
    return window.draw();
}
