/*! \file gzstream.hpp
 * \brief Flujos de salida y de entrada de texto con compresión gzip (zlib).
 *
 * ogzstream escribe un archivo .gz (o texto plano si \p comprimir es false,
 * con los mismos bytes que un std::ofstream) e igzstream lee archivos gzip
 * o de texto plano de forma transparente (gzopen detecta el formato).
 * Header-only; requiere enlazar con zlib.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <zlib.h>

#include <cstdio>
#include <fstream>
#include <istream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

namespace gz {

/// Búfer de salida que comprime con gzwrite.
class OutBuf : public std::streambuf {
public:
  OutBuf(const std::string &path, int level) : buf_(kSize) {
    file_ = gzopen(path.c_str(), ("wb" + std::to_string(level)).c_str());
    setp(buf_.data(), buf_.data() + buf_.size());
  }
  ~OutBuf() override { close(); }
  bool is_open() const { return file_ != nullptr; }
  void close() {
    if (file_) {
      flush_buf();
      gzclose(file_);
      file_ = nullptr;
    }
  }

protected:
  int_type overflow(int_type c) override {
    if (!flush_buf()) return traits_type::eof();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
    }
    return traits_type::not_eof(c);
  }
  int sync() override { return flush_buf() ? 0 : -1; }

private:
  static constexpr size_t kSize = 1 << 16;
  bool flush_buf() {
    const int n = static_cast<int>(pptr() - pbase());
    bool ok = true;
    if (n > 0 && file_) ok = gzwrite(file_, pbase(), n) == n;
    setp(buf_.data(), buf_.data() + buf_.size());
    return ok;
  }
  gzFile file_ = nullptr;
  std::vector<char> buf_;
};

/// Búfer de entrada que descomprime con gzread (o lee texto plano).
class InBuf : public std::streambuf {
public:
  explicit InBuf(const std::string &path) : buf_(kSize) {
    file_ = gzopen(path.c_str(), "rb");
    setg(buf_.data(), buf_.data(), buf_.data());
  }
  ~InBuf() override {
    if (file_) gzclose(file_);
  }
  bool is_open() const { return file_ != nullptr; }

protected:
  int_type underflow() override {
    if (!file_) return traits_type::eof();
    const int n = gzread(file_, buf_.data(), static_cast<unsigned>(kSize));
    if (n <= 0) return traits_type::eof();
    setg(buf_.data(), buf_.data(), buf_.data() + n);
    return traits_type::to_int_type(buf_[0]);
  }

private:
  static constexpr size_t kSize = 1 << 16;
  gzFile file_ = nullptr;
  std::vector<char> buf_;
};

} // namespace gz

/*! \class ogzstream
 * \brief std::ostream a un archivo, comprimido con gzip si \p comprimir.
 *
 * Con \p comprimir se le agrega ".gz" al nombre. path() devuelve el nombre
 * final. Se cierra al destruirse (o con close()).
 */
class ogzstream : public std::ostream {
public:
  ogzstream(const std::string &path, bool comprimir, int nivel = 6)
      : std::ostream(nullptr) {
    if (comprimir) {
      path_ = path + ".gz";
      auto b = std::make_unique<gz::OutBuf>(path_, nivel);
      ok_ = b->is_open();
      gzbuf_ = std::move(b);
      rdbuf(gzbuf_.get());
    } else {
      path_ = path;
      auto b = std::make_unique<std::filebuf>();
      ok_ = b->open(path_, std::ios::out) != nullptr;
      plainbuf_ = std::move(b);
      rdbuf(plainbuf_.get());
    }
    if (!ok_) setstate(std::ios::failbit);
  }
  ogzstream(const ogzstream &) = delete;
  ogzstream &operator=(const ogzstream &) = delete;
  ~ogzstream() override {
    flush();
    close();
  }
  bool is_open() const { return ok_; }
  const std::string &path() const { return path_; }
  void close() {
    if (gzbuf_) gzbuf_->close();
    if (plainbuf_) plainbuf_->close();
    ok_ = false;
  }

private:
  std::unique_ptr<gz::OutBuf> gzbuf_;
  std::unique_ptr<std::filebuf> plainbuf_;
  std::string path_;
  bool ok_ = false;
};

/*! \class igzstream
 * \brief std::istream desde un archivo gzip o de texto plano.
 */
class igzstream : public std::istream {
public:
  explicit igzstream(const std::string &path)
      : std::istream(nullptr), buf_(path) {
    rdbuf(&buf_);
    if (!buf_.is_open()) setstate(std::ios::failbit);
  }
  bool is_open() const { return buf_.is_open(); }

private:
  gz::InBuf buf_;
};
