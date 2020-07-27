#include "matplot_displayer.h"

#include "base/log/logging.h"
#include "matplotlibcpp.h"

#include <vector>

namespace display {

MathPlotDisplayer::MathPlotDisplayer() : running_(true) {
  matplotlibcpp::ion(); //开启一个画图的窗口
}

MathPlotDisplayer::~MathPlotDisplayer() {}

void MathPlotDisplayer::Close() {
  if (running_) {
    running_ = false;
    matplotlibcpp::close();
  }
}

void MathPlotDisplayer::OnRTTUpdate(const bool timeout,
                                    const uint64_t sequence_number_, int ttl) {
  rtt_index_.push_back(sequence_number_);
  rtt_value_.push_back(ttl);

  Refresh();
}

void MathPlotDisplayer::OnPacketLossUpdate(const uint64_t sequence_number_,
                                           const double loss) {
  loss_index_.push_back(sequence_number_);
  loss_value_.push_back(loss);

  Refresh();
}

void MathPlotDisplayer::Refresh() {
  if (running_) {
    matplotlibcpp::clf(); //清除之前的图
    matplotlibcpp::subplot(2, 1, 1);
    matplotlibcpp::title("Network State Monitor");
    matplotlibcpp::plot(rtt_index_, rtt_value_, "y"); // 画出当前图形
    matplotlibcpp::named_plot("rtt", rtt_index_, rtt_value_);
    matplotlibcpp::legend();

    matplotlibcpp::subplot(2, 1, 2);
    matplotlibcpp::plot(loss_index_, loss_value_); // 画出当前图形
    matplotlibcpp::named_plot("loss", loss_index_, loss_value_);

    matplotlibcpp::legend();
    matplotlibcpp::pause(0.01);
  }
}

} // namespace display
