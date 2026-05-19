#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct Options {
    std::string input;
    std::string output;
    std::string report;
    std::string csv;
    int width = 640;
    int height = 360;
    double fps = 25.0;
    int max_frames = 0;
    std::string backend = "demo-saliency";
};

struct Detection {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    float score = 0.0f;
    std::string label;
};

struct FrameTimings {
    double decode_ms = 0.0;
    double preprocess_ms = 0.0;
    double inference_ms = 0.0;
    double postprocess_ms = 0.0;
    double encode_ms = 0.0;
    double total_ms = 0.0;
    int detections = 0;
};

static double elapsed_ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static std::string quote_arg(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

static void ensure_parent_dir(const std::string& path) {
    fs::path p(path);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
}

static Options parse_args(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        auto need_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for " + name);
            }
            return argv[++i];
        };
        if (key == "--input") opt.input = need_value(key);
        else if (key == "--output") opt.output = need_value(key);
        else if (key == "--report") opt.report = need_value(key);
        else if (key == "--csv") opt.csv = need_value(key);
        else if (key == "--width") opt.width = std::stoi(need_value(key));
        else if (key == "--height") opt.height = std::stoi(need_value(key));
        else if (key == "--fps") opt.fps = std::stod(need_value(key));
        else if (key == "--max-frames") opt.max_frames = std::stoi(need_value(key));
        else if (key == "--backend") opt.backend = need_value(key);
        else if (key == "--help" || key == "-h") {
            std::cout
                << "local_cpp_video_pipeline --input in.mp4 --output out.mp4 [options]\n"
                << "  --width 640 --height 360 --fps 25 --max-frames 300\n"
                << "  --report report.md --csv latency.csv --backend demo-saliency\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + key);
        }
    }
    if (opt.input.empty()) throw std::runtime_error("--input is required");
    if (opt.output.empty()) throw std::runtime_error("--output is required");
    if (opt.report.empty()) opt.report = opt.output + ".report.md";
    if (opt.csv.empty()) opt.csv = opt.output + ".latency.csv";
    if (opt.width <= 0 || opt.height <= 0) throw std::runtime_error("Invalid frame size");
    if (opt.fps <= 0.0) throw std::runtime_error("Invalid fps");
    return opt;
}

class IInferenceBackend {
public:
    virtual ~IInferenceBackend() = default;
    virtual const char* name() const = 0;
    virtual std::vector<Detection> infer(const std::vector<unsigned char>& rgb, int width, int height) = 0;
};

class DemoSaliencyBackend final : public IInferenceBackend {
public:
    const char* name() const override { return "demo-saliency"; }

    std::vector<Detection> infer(const std::vector<unsigned char>& rgb, int width, int height) override {
        const int grid_x = 16;
        const int grid_y = 9;
        const int cell_w = std::max(8, width / grid_x);
        const int cell_h = std::max(8, height / grid_y);

        double global_mean = 0.0;
        for (int y = 0; y < height; y += 4) {
            for (int x = 0; x < width; x += 4) {
                const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
                global_mean += 0.299 * rgb[idx] + 0.587 * rgb[idx + 1] + 0.114 * rgb[idx + 2];
            }
        }
        const double samples = std::max(1, (height / 4) * (width / 4));
        global_mean /= samples;

        double best_score = -1.0;
        int best_x = 0;
        int best_y = 0;
        for (int cy = 0; cy < grid_y; ++cy) {
            for (int cx = 0; cx < grid_x; ++cx) {
                const int x0 = cx * cell_w;
                const int y0 = cy * cell_h;
                const int x1 = std::min(width, x0 + cell_w);
                const int y1 = std::min(height, y0 + cell_h);

                double mean = 0.0;
                double variance = 0.0;
                int count = 0;
                for (int y = y0; y < y1; y += 2) {
                    for (int x = x0; x < x1; x += 2) {
                        const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
                        const double luma = 0.299 * rgb[idx] + 0.587 * rgb[idx + 1] + 0.114 * rgb[idx + 2];
                        mean += luma;
                        ++count;
                    }
                }
                if (count == 0) continue;
                mean /= count;
                for (int y = y0; y < y1; y += 2) {
                    for (int x = x0; x < x1; x += 2) {
                        const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
                        const double luma = 0.299 * rgb[idx] + 0.587 * rgb[idx + 1] + 0.114 * rgb[idx + 2];
                        const double d = luma - mean;
                        variance += d * d;
                    }
                }
                variance /= count;
                const double contrast = std::abs(mean - global_mean);
                const double score = std::sqrt(variance) + 0.35 * contrast;
                if (score > best_score) {
                    best_score = score;
                    best_x = x0;
                    best_y = y0;
                }
            }
        }

        Detection det;
        det.x1 = std::clamp(best_x - cell_w / 2, 0, width - 1);
        det.y1 = std::clamp(best_y - cell_h / 2, 0, height - 1);
        det.x2 = std::clamp(best_x + cell_w * 2, 0, width - 1);
        det.y2 = std::clamp(best_y + cell_h * 2, 0, height - 1);
        det.score = static_cast<float>(std::clamp(best_score / 96.0, 0.05, 0.99));
        det.label = "demo-saliency";
        return {det};
    }
};

static std::unique_ptr<IInferenceBackend> create_backend(const std::string& name) {
    if (name == "demo-saliency") {
        return std::make_unique<DemoSaliencyBackend>();
    }
    throw std::runtime_error("Unsupported backend: " + name);
}

static void set_pixel(std::vector<unsigned char>& rgb, int width, int height, int x, int y,
                      unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
    rgb[idx] = r;
    rgb[idx + 1] = g;
    rgb[idx + 2] = b;
}

static void draw_rect(std::vector<unsigned char>& rgb, int width, int height, const Detection& det) {
    const int thickness = 3;
    for (int t = 0; t < thickness; ++t) {
        for (int x = det.x1; x <= det.x2; ++x) {
            set_pixel(rgb, width, height, x, det.y1 + t, 30, 230, 80);
            set_pixel(rgb, width, height, x, det.y2 - t, 30, 230, 80);
        }
        for (int y = det.y1; y <= det.y2; ++y) {
            set_pixel(rgb, width, height, det.x1 + t, y, 30, 230, 80);
            set_pixel(rgb, width, height, det.x2 - t, y, 30, 230, 80);
        }
    }
}

static void write_csv(const std::string& path, const std::vector<FrameTimings>& timings) {
    ensure_parent_dir(path);
    std::ofstream out(path);
    out << "frame,decode_ms,preprocess_ms,inference_ms,postprocess_ms,encode_ms,total_ms,detections\n";
    out << std::fixed << std::setprecision(3);
    for (size_t i = 0; i < timings.size(); ++i) {
        const auto& t = timings[i];
        out << i << ',' << t.decode_ms << ',' << t.preprocess_ms << ',' << t.inference_ms << ','
            << t.postprocess_ms << ',' << t.encode_ms << ',' << t.total_ms << ',' << t.detections << '\n';
    }
}

static double average_of(const std::vector<FrameTimings>& timings, double FrameTimings::*field) {
    if (timings.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& t : timings) sum += t.*field;
    return sum / static_cast<double>(timings.size());
}

static double percentile(std::vector<double> values, double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double pos = (static_cast<double>(values.size()) - 1.0) * p;
    const size_t lo = static_cast<size_t>(std::floor(pos));
    const size_t hi = static_cast<size_t>(std::ceil(pos));
    if (lo == hi) return values[lo];
    const double w = pos - static_cast<double>(lo);
    return values[lo] * (1.0 - w) + values[hi] * w;
}

static void write_report(const std::string& path, const Options& opt, const std::string& backend,
                         const std::vector<FrameTimings>& timings, double wall_ms) {
    ensure_parent_dir(path);
    std::vector<double> totals;
    totals.reserve(timings.size());
    for (const auto& t : timings) totals.push_back(t.total_ms);

    const double avg_total = average_of(timings, &FrameTimings::total_ms);
    const double fps = wall_ms > 0.0 ? static_cast<double>(timings.size()) * 1000.0 / wall_ms : 0.0;

    std::ofstream out(path);
    out << "# Local C++ Video Pipeline Report\n\n";
    out << "Backend: `" << backend << "`\n\n";
    out << "Note: `demo-saliency` is a C++ demo backend, not a neural network. It validates pipeline latency, drawing, and reporting before ONNX/OpenVINO/ncnn/MNN integration.\n\n";
    out << "## Input\n\n";
    out << "- Input: `" << opt.input << "`\n";
    out << "- Output: `" << opt.output << "`\n";
    out << "- Resolution: " << opt.width << "x" << opt.height << "\n";
    out << "- FPS setting: " << opt.fps << "\n";
    out << "- Frames processed: " << timings.size() << "\n\n";
    out << "## Latency\n\n";
    out << std::fixed << std::setprecision(3);
    out << "- Wall time: " << wall_ms << " ms\n";
    out << "- Throughput: " << fps << " fps\n";
    out << "- Avg total/frame: " << avg_total << " ms\n";
    out << "- P50 total/frame: " << percentile(totals, 0.50) << " ms\n";
    out << "- P90 total/frame: " << percentile(totals, 0.90) << " ms\n";
    out << "- Avg decode: " << average_of(timings, &FrameTimings::decode_ms) << " ms\n";
    out << "- Avg preprocess: " << average_of(timings, &FrameTimings::preprocess_ms) << " ms\n";
    out << "- Avg inference: " << average_of(timings, &FrameTimings::inference_ms) << " ms\n";
    out << "- Avg postprocess: " << average_of(timings, &FrameTimings::postprocess_ms) << " ms\n";
    out << "- Avg encode write: " << average_of(timings, &FrameTimings::encode_ms) << " ms\n\n";
    out << "## Next Backend Targets\n\n";
    out << "- ONNX Runtime C++ CPU baseline.\n";
    out << "- OpenVINO CPU/GPU path for Intel hardware.\n";
    out << "- ncnn and MNN edge comparison.\n";
}

int main(int argc, char** argv) {
    try {
        const Options opt = parse_args(argc, argv);
        auto backend = create_backend(opt.backend);

        ensure_parent_dir(opt.output);
        ensure_parent_dir(opt.report);
        ensure_parent_dir(opt.csv);

        const size_t frame_bytes = static_cast<size_t>(opt.width) * opt.height * 3;
        std::vector<unsigned char> frame(frame_bytes);
        std::vector<FrameTimings> timings;

        std::ostringstream decode_cmd;
        decode_cmd << "ffmpeg -hide_banner -loglevel error -i " << quote_arg(opt.input)
                   << " -vf scale=" << opt.width << ':' << opt.height
                   << " -pix_fmt rgb24";
        if (opt.max_frames > 0) {
            decode_cmd << " -frames:v " << opt.max_frames;
        }
        decode_cmd << " -f rawvideo pipe:1";

        std::ostringstream encode_cmd;
        encode_cmd << "ffmpeg -hide_banner -loglevel error -y"
                   << " -f rawvideo -pix_fmt rgb24 -s " << opt.width << 'x' << opt.height
                   << " -r " << opt.fps
                   << " -i pipe:0 -an -c:v libx264 -preset veryfast -crf 23 -pix_fmt yuv420p "
                   << quote_arg(opt.output);

        FILE* decoder = popen(decode_cmd.str().c_str(), "rb");
        if (!decoder) throw std::runtime_error("Failed to start decoder ffmpeg process");

        FILE* encoder = popen(encode_cmd.str().c_str(), "wb");
        if (!encoder) {
            pclose(decoder);
            throw std::runtime_error("Failed to start encoder ffmpeg process");
        }

        const auto wall_start = Clock::now();
        int frame_index = 0;
        while (opt.max_frames <= 0 || frame_index < opt.max_frames) {
            FrameTimings ft;
            const auto frame_start = Clock::now();

            const auto t0 = Clock::now();
            const size_t got = std::fread(frame.data(), 1, frame_bytes, decoder);
            const auto t1 = Clock::now();
            ft.decode_ms = elapsed_ms(t0, t1);
            if (got == 0) break;
            if (got != frame_bytes) {
                std::cerr << "Partial frame received, stopping. bytes=" << got << "\n";
                break;
            }

            const auto p0 = Clock::now();
            // Preprocess placeholder. The demo backend consumes RGB directly.
            const auto p1 = Clock::now();
            ft.preprocess_ms = elapsed_ms(p0, p1);

            const auto i0 = Clock::now();
            auto detections = backend->infer(frame, opt.width, opt.height);
            const auto i1 = Clock::now();
            ft.inference_ms = elapsed_ms(i0, i1);
            ft.detections = static_cast<int>(detections.size());

            const auto pp0 = Clock::now();
            for (const auto& det : detections) draw_rect(frame, opt.width, opt.height, det);
            const auto pp1 = Clock::now();
            ft.postprocess_ms = elapsed_ms(pp0, pp1);

            const auto e0 = Clock::now();
            const size_t written = std::fwrite(frame.data(), 1, frame_bytes, encoder);
            const auto e1 = Clock::now();
            ft.encode_ms = elapsed_ms(e0, e1);
            if (written != frame_bytes) {
                std::cerr << "Encoder pipe write failed, stopping.\n";
                break;
            }

            const auto frame_end = Clock::now();
            ft.total_ms = elapsed_ms(frame_start, frame_end);
            timings.push_back(ft);
            ++frame_index;
        }

        const auto wall_end = Clock::now();
        const double wall_ms = elapsed_ms(wall_start, wall_end);

        const int decoder_status = pclose(decoder);
        const int encoder_status = pclose(encoder);
        if (decoder_status != 0) {
            std::cerr << "Decoder exited with status " << decoder_status << "\n";
        }
        if (encoder_status != 0) {
            std::cerr << "Encoder exited with status " << encoder_status << "\n";
        }

        write_csv(opt.csv, timings);
        write_report(opt.report, opt, backend->name(), timings, wall_ms);

        std::cout << "frames=" << timings.size()
                  << " avg_total_ms=" << std::fixed << std::setprecision(3)
                  << average_of(timings, &FrameTimings::total_ms)
                  << " report=" << opt.report
                  << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
