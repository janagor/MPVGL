#include <glm/gtc/matrix_transform.hpp>

#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Geometry/Shape.hpp"

int main() {
    mpvgl::Scene scene{};

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::Literal::Red),
              glm::translate(glm::mat4(1.0F), glm::vec3(-2.0F, 0.0F, 0.0F)));

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::Literal::Green),
              glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F)));

    scene.add(mpvgl::Shape<mpvgl::Quad>::generate(mpvgl::Color::Literal::Blue),
              glm::translate(glm::mat4(1.0F), glm::vec3(2.0F, 1.0F, 0.0F)));

    mpvgl::RenderWindow window(1280, 720, "Mój Super Silnik", scene);
    return window.draw();
}
