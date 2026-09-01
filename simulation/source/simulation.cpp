#include <cassert>
#include <cmath>
#include <numeric>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <array>
#include <queue>
#include <optional>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/cast.h>
#include <pybind11/stl.h>
#include <raylib.h>

#include "simulation.hpp"
#include "renderer.hpp"
#include "controller.hpp"
#include "vec2.hpp"
#include "missile.hpp"

using namespace simulation;
namespace py = pybind11;

using ActType = std::tuple<bool, bool, bool>;

class PythonSimulation {
private:
    Simulation sim_;
    const int actionPeriod_;

    std::optional<Size> windowSize_ = std::nullopt;
    std::queue<std::tuple<Simulation, ActType>> renderingQueue;

public:
    PythonSimulation(
        size_t timeout,
        int actionPeriod = 2,
        std::optional<std::tuple<size_t, size_t>> windowSize = std::nullopt
    ): sim_(timeout), actionPeriod_(actionPeriod)
    {
        if (actionPeriod_ <= 0) {
            throw  std::runtime_error("action_period must be bigger than zero");
        }


        if (windowSize.has_value()) {
            if (IsWindowReady()) {
                throw std::runtime_error("Renderer Instance is Already exist");
            }

            auto [width, height] = (*windowSize);
            windowSize_ = {width, height};

            InitWindow( (int) width, (int) height, "Raylib Renderer");
            SetTargetFPS(120);
        }
    }

    py::array_t<float> getObs() const {
        py::array_t<float> obs(12);
        float* ptr = obs.mutable_data();

        const Rocket& rocket = sim_.rocket();
        const Missile& missile = sim_.missile();
        const Size spaceSize = sim_.spaceSize();
        // pos
        ptr[0] = rocket.transform().pos.x / static_cast<float>(spaceSize.width);
        ptr[1] = rocket.transform().pos.y / static_cast<float>(spaceSize.height);
        ptr[2] = std::tanh(rocket.rb().linearVel().x / 60);
        ptr[3] = std::tanh(rocket.rb().linearVel().y / 60);
        
        // angle
        ptr[4] = std::sin(rocket.transform().angle);
        ptr[5] = std::cos(rocket.transform().angle);
        ptr[6] = std::tanh(rocket.rb().angularVel() / 5);

        //missile
        ptr[7] = missile.alive() ? 1 : 0;
        ptr[8] = (missile.transform().centerX() - rocket.transform().centerX()) / static_cast<float>(spaceSize.width);
        ptr[9] = (missile.transform().centerY() - rocket.transform().centerY()) / static_cast<float>(spaceSize.height);
        ptr[10] = std::tanh((missile.rb().linearVel().x - rocket.rb().linearVel().x) / 180);
        ptr[11] = std::tanh((missile.rb().linearVel().y - rocket.rb().linearVel().y) / 180);

        return obs;
    }

    bool getTerminated() const {
        return sim_.isTerminated();
    }

    bool getTruncated() const {
        return sim_.isTruncated();
    }

    std::array<float, 5> getRewardList() const {
        float spaceDiagonal = std::sqrt(static_cast<float>(sim_.spaceSize().width*sim_.spaceSize().width + sim_.spaceSize().height*sim_.spaceSize().height));

        float xRatio = sim_.rocket().transform().centerX() / static_cast<float>(sim_.spaceSize().width);
        float yRatio = sim_.rocket().transform().centerY() / static_cast<float>(sim_.spaceSize().height);
        float missileDistRatio = (sim_.rocket().transform().center() - sim_.missile().transform().center()).length() / spaceDiagonal;

        std::array<float, 5> reward = {
            1.0f,
            0.5f - 2 * std::fabs(0.5f - xRatio),
            0.5f - 2 * std::fabs(0.5f - yRatio),
            0.25f * std::cos(sim_.rocket().transform().angle),
            sim_.missile().alive() ? 0.5f * (missileDistRatio - 0.5f) : 0.0f
        };
        return reward;
    }


    float getReward() const {
        auto list = getRewardList();

        if (!getTerminated()) {
            return std::accumulate(list.begin(), list.end(), 0.0f) / 10;
        }
        else {
            return -10;
        }
        
    }

    void step(const py::array_t<int>& action) {

        // get cpp action
        assert(action.shape(0) == 3);
        const int* actionPtr = action.data();
        const auto cppAction = std::tuple {
            static_cast<bool>(actionPtr[0]),
            static_cast<bool>(actionPtr[1]),
            static_cast<bool>(actionPtr[2])
        };
        
        // simulate
        for (int i = 0; i < actionPeriod_; i++) {
            sim_.update(cppAction);
            
            // rendering
            if (windowSize_.has_value()) {
                if (renderingQueue.size() >= actionPeriod_) {
                    renderingQueue.pop();
                }
                renderingQueue.emplace(sim_, cppAction);
            }

            if (sim_.isTerminated() || sim_.isTruncated()) return;
        }
    }

    void reset(std::optional<unsigned int> seed = std::nullopt) {
        sim_.reset(seed);

        // rendering
        while (!renderingQueue.empty()) renderingQueue.pop();
    }

    void render() {
        if (!windowSize_.has_value()) {
            throw std::runtime_error("This Instance is not rendering instacne");
        }

        while (!renderingQueue.empty()) {
            auto [sim, action] = renderingQueue.front();
            frontend::render(*windowSize_, sim, action);
            renderingQueue.pop();
        }
    }

    void close() {
        if (windowSize_.has_value()) {
            CloseWindow();
            windowSize_ = std::nullopt; 
        }
        
    }


};

PYBIND11_MODULE(simulation, m) {
    py::class_<PythonSimulation>(m, "Simulation")
        .def(
            py::init<size_t, int, std::optional<std::tuple<size_t, size_t>>>(),
            py::arg("timeout"), py::arg("action_period") = 2, py::arg("window_size") = std::nullopt
        )
        .def("get_obs", &PythonSimulation::getObs)
        .def("get_terminated", &PythonSimulation::getTerminated)
        .def("get_truncated", &PythonSimulation::getTruncated)
        .def("get_reward_list", &PythonSimulation::getRewardList)
        .def("get_reward", &PythonSimulation::getReward)
        .def("step", &PythonSimulation::step, py::arg("action"))
        .def("reset", &PythonSimulation::reset, py::arg("seed") = std::nullopt)
        .def("render", &PythonSimulation::render)
        .def("close", &PythonSimulation::close);

    m.def("get_action_from_keyboard", &frontend::getActionFromKeyboard);
    m.def("window_should_close", &WindowShouldClose);
}