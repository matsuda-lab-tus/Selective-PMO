#include "pmo.h"
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
  const auto config = pmo::Config(argc, argv);

  const auto radius = config.size_template_radius<int>();
  const auto num_examples = config.num_examples<int>();
  const auto num_predictors = config.num_predictors<int>();
  const auto size_search_window = config.size_search_window<int>();
  const auto size_train_window = config.size_training_window<int>();
  const auto num_context_segments = config.num_context_segments<int>();
  const auto penalty = config.template_weight_penalty<double>();
  constexpr auto is_parallel = false;

  if (config.is_encoder()) {
    auto start_encoder = std::chrono::system_clock::now();

    const auto &input = config.path_input_image();
    const auto &bitstream = config.path_bitstream();

    auto image = pmo::Image_<unsigned char>(input);
    auto template_patch = pmo::TemplatePatch(radius, 0);

    auto basic_param_map = pmo::BasicParameterMap(image.height(), image.width(), num_examples + num_predictors);
    auto model_param_map = pmo::ModelParameterMap(image.height(), image.width(), num_context_segments);
    auto context_param_map = pmo::ContextParameterMap_<unsigned char>(image.height(), image.width(), template_patch);

    auto encoder = pmo::Encoder(bitstream, image, template_patch, basic_param_map, model_param_map, context_param_map);

    {// Estimate basic parameter
      std::cout << "[[Start: Estimate basic parameter]]" << std::endl;
      auto time_example_search = std::chrono::microseconds{};
      auto time_adaptive_prediction = std::chrono::microseconds{};

      auto example_search = pmo::ExampleSearch(image, template_patch, basic_param_map);
      auto adaptive_prediction = pmo::AdaptivePrediction(image, basic_param_map, is_parallel);

      for (auto y = 0; y < image.height(); ++y) {
        for (auto x = 0; x < image.width(); ++x) {
          const auto target = pmo::Point(x, y);

          {// Example Search
            const auto start = std::chrono::high_resolution_clock::now();

            example_search.estimate(target, num_examples, size_search_window, penalty);

            const auto end = std::chrono::high_resolution_clock::now();

            time_example_search += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
          }//

          {// Adaptive Prediction
            const auto start = std::chrono::high_resolution_clock::now();

            adaptive_prediction.estimate(target, num_examples + num_predictors, size_train_window);

            const auto end = std::chrono::high_resolution_clock::now();

            time_adaptive_prediction += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
          }//

          example_search.update_template(target);
        }
      }

      std::cout << "[[Finish: Example Search]]" << static_cast<double>(time_example_search.count()) / 1000000 << " sec." << std::endl;
      std::cout << "[[Finish: Adaptive Prediction]]" << static_cast<double>(time_adaptive_prediction.count()) / 1000000 << " sec." << std::endl;
      std::cout << "[[Finish: Estimate basic parameter]]" << std::endl;
    }//

    {// Optimize model parameter
      std::cout << "[[Start: Optimize model parameter]]" << std::endl;
      const auto start = std::chrono::system_clock::now();

      auto optimizer = pmo::Optimizer(image, basic_param_map, model_param_map, context_param_map);

      optimizer.optimize();

      const auto finish = std::chrono::system_clock::now();
      const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
      std::cout << "[[Finish: Optimize model parameter]] " << static_cast<double>(time) / 1000 << " sec." << std::endl;
    }//

    {// Encode
      std::cout << "[[Start: Encode]]" << std::endl;

      encoder.encode_start();

      for (auto unit_id = 0; unit_id < model_param_map.num_units(); ++unit_id)
        encoder.encode_model_parameter(unit_id);

      for (auto y = 0; y < image.height(); ++y)
        for (auto x = 0; x < image.width(); ++x)
          encoder.encode_pix({x, y});

      encoder.encode_finish();

      std::cout << "[[Finish: Encode]]" << std::endl;
    }//

    auto finish_encoder = std::chrono::system_clock::now();
    const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(finish_encoder - start_encoder).count();
    std::cout << "Encoder CPU time " << static_cast<double>(time) / 1000 << " sec." << std::endl;
  }

  if (config.is_decoder()) {
    auto start_decoder = std::chrono::system_clock::now();

    const auto &bitstream = config.path_bitstream();
    const auto &output = config.path_output_image();

    auto image = pmo::Image_<unsigned char>();
    auto template_patch = pmo::TemplatePatch();

    auto basic_param_map = pmo::BasicParameterMap();
    auto model_param_map = pmo::ModelParameterMap();
    auto context_param_map = pmo::ContextParameterMap_<unsigned char>(template_patch);

    auto decoder = pmo::Decoder(bitstream, image, template_patch, basic_param_map, model_param_map, context_param_map);

    {// Estimate basic parameter
      std::cout << "[[Start: Decode]]" << std::endl;

      decoder.decode_start();

      for (auto unit_id = 0; unit_id < model_param_map.num_units(); ++unit_id)
        decoder.decode_model_parameter(unit_id);

      auto example_search = pmo::ExampleSearch(image, template_patch, basic_param_map);
      auto adaptive_prediction = pmo::AdaptivePrediction(image, basic_param_map, is_parallel);

      for (auto y = 0; y < image.height(); ++y) {
        for (auto x = 0; x < image.width(); ++x) {
          const auto target = pmo::Point(x, y);

          {// Example Search
            example_search.estimate(target, num_examples, size_search_window, penalty);
          }//

          {// Adaptive Prediction
            adaptive_prediction.estimate(target, num_examples + num_predictors, size_train_window);
          }//

          decoder.decode_pix(target);

          example_search.update_template(target);
        }
      }
      decoder.decode_finish();

      image.save(output);

      std::cout << "[[Finish: Decode]]" << std::endl;
    }//

    auto finish_decoder = std::chrono::system_clock::now();
    const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(finish_decoder - start_decoder).count();
    std::cout << "Decoder CPU time " << static_cast<double>(time) / 1000 << " sec." << std::endl;
  }

  if (config.is_encoder() && config.is_decoder()) {
    std::cout << "[[Start: Check]]" << std::endl;

    const auto &input = config.path_input_image();
    const auto &output = config.path_output_image();

    const auto image_i = pmo::Image_<unsigned char>(input);
    const auto image_o = pmo::Image_<unsigned char>(output);

    const auto &data_i = image_i.data();
    const auto &data_o = image_o.data();

    const auto equal = data_i.size() == data_o.size() && std::equal(data_i.cbegin(), data_i.cend(), data_o.cbegin());

    if (equal)
      std::cout << "Successful lossless compression." << std::endl;
    else
      std::cout << "Sorry. Lossless compression failed." << std::endl;

    std::cout << "[[Finish: Check]]" << std::endl;
  }

  exit(EXIT_SUCCESS);
}
