// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
// 功能:根据时间切分日志

#pragma once

#include <spdlog/common.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/os.h>
#include <spdlog/details/circular_q.h>
#include <spdlog/details/synchronous_factory.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <type_traits>
#include <queue>


using namespace spdlog;

namespace spdlog {
namespace sinks {

/* 根据时间决定日志截断逻辑
 * 默认时间切分单位是秒 */
template<typename Mutex>
class time_split_file_sink final : public base_sink<Mutex> {
public:
	/* 构造函数
	 * base_filename,       日志文件名称
	 * split_time           日志切分时间, 默认10分钟切分一次日志, 单位: 秒
	 * max_files = 24,      最大日志缓存数量, 默认 6*24*7 = 1,008, 默认保存一周的日志
	 * event_handlers = {}  事件处理
	 * */
	time_split_file_sink(filename_t base_filename, int split_time = 60 * 10, size_t max_files = 1008,
	                     const file_event_handlers &event_handlers = {})
			: base_filename_(base_filename), split_time_(split_time),
			  max_files_(max_files), file_helper_{event_handlers} {

		update_current_log_file(); // 更新第一次保存的日志名字

		// 打开日志文件 默认一直只打开当前文件
		file_helper_.open(base_filename_);
	}

	filename_t filename() {
		std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
		return file_helper_.filename();
	}

	// 立即更新当前日志, 但会本次保存的日志文件名
	std::string Flush() {
		rotate_();  // 保存当前日志并打开新文件
		return last_save_file_;
	}

protected:
	// 写入日志
	void sink_it_(const details::log_msg &msg) override {
		auto time = msg.time;  // 写入日志时间

		// 判断是否要切割日志文件
		log_clock::time_point need_split_time = std::chrono::seconds(split_time_) + last_save_log_time_;
		if (time >= need_split_time) {
			file_helper_.flush();  // 保存日志
			if (file_helper_.size() > 0) {
				rotate_();  // 保存当前日志并打开新文件
			}
		}

		// 格式化日志并写入文件
		memory_buf_t formatted;
		base_sink<Mutex>::formatter_->format(msg, formatted);
		file_helper_.write(formatted);
	}

	// 立即将日志写入文件
	void flush_() override {
		file_helper_.flush();
	}

private:
	// 保存当前日志并打开新文件
	void rotate_() {
		std::unique_lock<std::mutex> locker(_mutex);
		using details::os::filename_to_str;
		using details::os::path_exists;

		file_helper_.close();// 关闭当前文件

		// 通过重命名的方式保存数据
		if (!rename_file_(base_filename_, current_save_file_)) {
			details::os::sleep_for_millis(100);  // 防止切分日志太快导致失败
			if (!rename_file_(base_filename_, current_save_file_)) {
				// 无论如何都要截断日志文件，以防止其增长超过限制！
				file_helper_.reopen(true);
				throw_spdlog_ex("rotating_file_sink: failed renaming " + filename_to_str(base_filename_) +
				                " to " + filename_to_str(current_save_file_), errno);
			}
		}

		update_current_log_file(); // 更新下一次保存的日志名字
		file_helper_.reopen(true);
	}

	// 重命名文件
	inline bool rename_file_(const filename_t &src_filename, const filename_t &target_filename) {
		// 如果目标文件已经存在，尝试删除。
		(void) details::os::remove(target_filename);
		return details::os::rename(src_filename, target_filename) == 0;
	}

	// 更新当前日志文件
	inline void update_current_log_file() {
		// 本次保存日志时间
		last_save_log_time_ = get_current_time();
		// 获取当前时间
		log_clock::time_point now = last_save_log_time_;

		// 根据时间生成日志文件名
		// current_save_file_ = calc_filename(base_filename_, now_tm(now));
		last_save_file_ = current_save_file_;  // 缓存上一次保存的文件名
		current_save_file_ = calc_filename_with_timeStamp(base_filename_, now);

		// 缓存日志文件已经达到最大值
		if (file_buffer_.size() > max_files_) {
			// 删除早期的日志文件
			filename_t del_file = file_buffer_.front();
			file_buffer_.pop();
			(void) details::os::remove(del_file);
		}
		file_buffer_.push(current_save_file_);  // 入队
	}

	// 时间戳转换: 时间戳(秒) -> 格式
	tm now_tm(log_clock::time_point tp) {
		time_t tnow = log_clock::to_time_t(tp);
		return spdlog::details::os::localtime(tnow);
	}

	// 获取当前时间
	inline log_clock::time_point get_current_time() {
		auto now = log_clock::now();  // 当前时间time_point

		time_t tnow = log_clock::to_time_t(now);  //  time_point 转换成 time_t秒
		tm date = spdlog::details::os::localtime(tnow);

		auto rotation_time = log_clock::from_time_t(std::mktime(&date)); // 从 time_t 转换成 time_point
		if (rotation_time > now) {
			return rotation_time;
		}
		return rotation_time;
	}

	// 根据时间戳获取日志文件名
	static filename_t calc_filename(const filename_t &filename, const tm &now_tm) {
		filename_t basename, ext;
		std::tie(basename, ext) = details::file_helper::split_by_extension(filename);

		// log_file_name_2022-03-29-14h5min.log
		return fmt_lib::format(
				SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}-{:02d}h{:02d}min{}"),
				basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday,
				now_tm.tm_hour, now_tm.tm_min, ext);
	}

	// 根据时间戳获取日志文件名
	static filename_t calc_filename_with_timeStamp(const filename_t &filename, const log_clock::time_point &tp) {
		filename_t basename, ext;
		std::tie(basename, ext) = details::file_helper::split_by_extension(filename);

		// log_file_name_时间戳(秒).log
		std::time_t tt;
		tt = log_clock::system_clock::to_time_t(tp);
		return fmt_lib::format(SPDLOG_FILENAME_T("{}_{:10d}{}"), basename, tt, ext);
	}

	std::mutex _mutex;                 // 互斥量

	filename_t base_filename_;           // 日志文件名前缀
	details::file_helper file_helper_;   // 文件操作类
	std::size_t max_files_;              // 最大缓存文件数量
	int split_time_;                     // 日志文件的时间差值, 单位: 秒
	filename_t current_save_file_;       // 当前应该保存的文件名
	filename_t last_save_file_;          // 上一次保存的文件名

	log_clock::time_point last_save_log_time_;  // 上一次保存日志的时间
	std::queue<filename_t> file_buffer_;        // 缓存已经写入的日志名
};

// 模板别名
using time_split_file_sink_mt = time_split_file_sink<std::mutex>;
using time_split_file_sink_st = time_split_file_sink<details::null_mutex>;
} // namespace sinks

//
// factory functions
// rotating_logger_mt 表示用于多线程场合
// rotating_logger_st 表示用于单线程场合
// 


template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger>
time_split_logger_mt(const std::string &logger_name, const filename_t &filename, int split_time,
                     uint16_t max_files = 0, const file_event_handlers &event_handlers = {}) {
	return Factory::template create<sinks::time_split_file_sink_mt>(
			logger_name, filename, split_time, max_files, event_handlers);
}

template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger>
time_split_logger_st(const std::string &logger_name, const filename_t &filename, int split_time,
                     uint16_t max_files = 0, const file_event_handlers &event_handlers = {}) {
	return Factory::template create<sinks::time_split_file_sink_st>(
			logger_name, filename, split_time, max_files, event_handlers);
}

} // namespace spdlog
