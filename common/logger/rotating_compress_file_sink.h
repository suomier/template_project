#ifndef ROTATING_COMPRESS_FILE_SINK_H
#define ROTATING_COMPRESS_FILE_SINK_H

/**
 * @file rotating_compress_file_sink.h
 * @brief 支持压缩的旋转文件日志 sink 实现
 *
 * 提供基于 spdlog 的日志文件旋转功能，当日志文件达到指定大小时自动旋转，
 * 并支持对旧日志文件进行压缩以节省存储空间。
 */

#include <mutex>
#include <string>

#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>

using namespace spdlog;

namespace spdlog
{
namespace sinks
{
    /**
     * @brief 支持压缩的旋转文件日志 sink
     * @tparam Mutex 互斥锁类型，用于线程安全
     *
     * 当日志文件大小达到 max_size 时，会自动进行旋转，并支持压缩旧日志文件
     */
    template <typename Mutex>
    class rotating_compress_file_sink final : public base_sink<Mutex>
    {
    public:
        /** @brief 最大文件数量限制 */
        static constexpr size_t MaxFiles = 200000;

        /**
         * @brief 构造函数
         * @param base_filename 基础文件名
         * @param max_size 单个日志文件最大大小（字节）
         * @param max_files 最大保留文件数量
         * @param rotate_on_open 打开时是否旋转
         * @param event_handlers 文件事件处理器
         */
        rotating_compress_file_sink(filename_t base_filename,
                                    std::size_t max_size,
                                    std::size_t max_files,
                                    bool rotate_on_open = false,
                                    const file_event_handlers &event_handlers = {})
        {
            if (max_size == 0)
            {
                throw_spdlog_ex("rotating sink constructor: max_size arg cannot be zero");
            }

            if (max_files > MaxFiles)
            {
                throw_spdlog_ex("rotating sink constructor: max_files arg cannot exceed MaxFiles");
            }
            file_helper_.open(calc_filename(base_filename_, 0));
            current_size_ = file_helper_.size(); // expensive. called only once
            if (rotate_on_open && current_size_ > 0)
            {
                rotate_();
                current_size_ = 0;
            }
        }

        /**
         * @brief 计算带索引的日志文件名
         * @param filename 基础文件名
         * @param index 文件索引
         * @return 计算后的文件名
         */
        static filename_t calc_filename(const filename_t &filename, std::size_t index)
        {
            if (index == 0U)
            {
                return filename;
            }

            filename_t basename;
            filename_t ext;
            std::tie(basename, ext) = details::file_helper::split_by_extension(filename);
            return fmt_lib::format(SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}.{}{}")), basename, index, ext);
        }

        /**
         * @brief 获取当前日志文件名
         * @return 当前日志文件名
         */
        filename_t filename()
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            return file_helper_.filename();
        }

        /**
         * @brief 立即执行日志文件旋转
         */
        void rotate_now()
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            rotate_();
        }

        /**
         * @brief 设置单个日志文件最大大小
         * @param max_size 最大大小（字节）
         */
        void set_max_size(std::size_t max_size)
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            if (max_size == 0)
            {
                throw_spdlog_ex("rotating sink set_max_size: max_size arg cannot be zero");
            }
            max_size_ = max_size;
        }

        /**
         * @brief 获取单个日志文件最大大小
         * @return 最大大小（字节）
         */
        std::size_t get_max_size()
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            return max_size_;
        }

        /**
         * @brief 设置最大保留文件数量
         * @param max_files 最大文件数量
         */
        void set_max_files(std::size_t max_files)
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            if (max_files > MaxFiles)
            {
                throw_spdlog_ex("rotating sink set_max_files: max_files arg cannot exceed 200000");
            }
            max_files_ = max_files;
        }

        /**
         * @brief 获取最大保留文件数量
         * @return 最大文件数量
         */
        std::size_t get_max_files()
        {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            return max_files_;
        }

    protected:
        /**
         * @brief 写入日志消息
         * @param msg 日志消息
         */
        void sink_it_(const details::log_msg &msg) override
        {
            memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);
            auto new_size = current_size_ + formatted.size();

            // 如果新的估算文件大小超过最大大小则旋转
            // 仅在实际大小 > 0 时旋转，以更好地处理磁盘已满的情况（参见 issue #2261）
            // 仅在 new_size > max_size_ 时检查实际大小，因为这相对昂贵
            if (new_size > max_size_)
            {
                file_helper_.flush();
                if (file_helper_.size() > 0)
                {
                    rotate_();
                    new_size = formatted.size();
                }
            }
            file_helper_.write(formatted);
            current_size_ = new_size;
        }

        /**
         * @brief 刷新日志缓冲区
         */
        void flush_() override
        {
            file_helper_.flush();
        }

    private:
        /**
         * @brief 执行日志文件旋转
         *
         * 旋转规则：
         * log.txt -> log.1.txt
         * log.1.txt -> log.2.txt
         * log.2.txt -> log.3.txt
         * log.3.txt -> 删除
         */
        void rotate_()
        {
            using details::os::filename_to_str;
            using details::os::path_exists;

            file_helper_.close();
            for (auto i = max_files_; i > 0; --i)
            {
                filename_t src = calc_filename(base_filename_, i - 1);
                if (!path_exists(src))
                {
                    continue;
                }
                filename_t target = calc_filename(base_filename_, i);

                if (!rename_file_(src, target))
                {
                    // 如果失败，在短暂延迟后重试
                    // 这是一个 Windows 问题的变通方法，非常高的旋转率可能导致
                    // 重命名失败并提示权限拒绝（可能是由于杀毒软件？）
                    details::os::sleep_for_millis(100);
                    if (!rename_file_(src, target))
                    {
                        file_helper_.reopen(true); // 截断日志文件以防止其超出限制！
                        current_size_ = 0;

                        throw_spdlog_ex("rotating_file_sink: failed renaming " + filename_to_str(src) + " to " + filename_to_str(target), errno);
                    }
                }
            }
            file_helper_.reopen(true);
        }

        /**
         * @brief 重命名文件
         * @param src_filename 源文件名
         * @param target_filename 目标文件名
         * @return 成功返回 true，失败返回 false
         *
         * 删除目标文件（如果存在），然后将源文件重命名为目标文件
         */
        bool rename_file_(const filename_t &src_filename, const filename_t &target_filename)
        {
            // 尝试删除目标文件（如果已存在）
            (void)details::os::remove(target_filename);
            return details::os::rename(src_filename, target_filename) == 0;
        }

        /** @brief 基础文件名 */
        filename_t base_filename_;
        /** @brief 单个日志文件最大大小（字节） */
        std::size_t max_size_;
        /** @brief 最大保留文件数量 */
        std::size_t max_files_;
        /** @brief 当前文件大小（字节） */
        std::size_t current_size_;
        /** @brief 文件操作辅助类 */
        details::file_helper file_helper_;
    };

    /** @brief 线程安全的旋转压缩文件 sink 别名 */
    using rotating_compress_file_sink_mt = rotating_compress_file_sink<std::mutex>;
    /** @brief 单线程的旋转压缩文件 sink 别名 */
    using rotating_compress_file_sink_st = rotating_compress_file_sink<details::null_mutex>;

} // namespace sinks

//
// 工厂函数
//
/**
 * @brief 创建线程安全的旋转压缩文件日志记录器
 * @tparam Factory 工厂类型
 * @param logger_name 日志记录器名称
 * @param filename 日志文件名
 * @param max_file_size 单个文件最大大小（字节）
 * @param max_files 最大文件数量
 * @param rotate_on_open 打开时是否旋转
 * @param event_handlers 文件事件处理器
 * @return 日志记录器共享指针
 */
template <typename Factory = spdlog::synchronous_factory>
std::shared_ptr<logger> rotating_logger_mt(const std::string &logger_name,
                                           const filename_t &filename,
                                           size_t max_file_size,
                                           size_t max_files,
                                           bool rotate_on_open = false,
                                           const file_event_handlers &event_handlers = {})
{
    return Factory::template create<sinks::rotating_compress_file_sink_mt>(
        logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}

/**
 * @brief 创建单线程的旋转压缩文件日志记录器
 * @tparam Factory 工厂类型
 * @param logger_name 日志记录器名称
 * @param filename 日志文件名
 * @param max_file_size 单个文件最大大小（字节）
 * @param max_files 最大文件数量
 * @param rotate_on_open 打开时是否旋转
 * @param event_handlers 文件事件处理器
 * @return 日志记录器共享指针
 */
template <typename Factory = spdlog::synchronous_factory>
std::shared_ptr<logger> rotating_logger_st(const std::string &logger_name,
                                           const filename_t &filename,
                                           size_t max_file_size,
                                           size_t max_files,
                                           bool rotate_on_open = false,
                                           const file_event_handlers &event_handlers = {})
{
    return Factory::template create<sinks::rotating_compress_file_sink_st>(
        logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}
} // namespace spdlog

#endif // ROTATING_COMPRESS_FILE_SINK_H
