#pragma once

#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <srl.hpp>

namespace
{
  constexpr size_t kBufferSize = 255;
  constexpr uint8_t kDisplayColumns = 44;
  constexpr uint8_t kDisplayLines = 27;
  char results_buffer[kBufferSize] = {};

  struct ResultLine
  {
    char text[kBufferSize];
  };

  std::vector<ResultLine> g_results;

  void AppendChar(char *dst, const size_t size, size_t &pos, const char ch)
  {
    if (pos + 1 < size)
    {
      dst[pos] = ch;
    }
    ++pos;
  }

  void AppendStr(char *dst, const size_t size, size_t &pos, const char *src)
  {
    if (!src)
    {
      return;
    }

    for (size_t i = 0; src[i] != '\0'; ++i)
    {
      AppendChar(dst, size, pos, src[i]);
    }
  }

  void AppendUnsigned(char *dst, const size_t size, size_t &pos, unsigned int value)
  {
    char tmp[10];
    size_t tmp_len = 0;

    if (value == 0)
    {
      tmp[tmp_len++] = '0';
    }
    else
    {
      while (value > 0 && tmp_len < sizeof(tmp))
      {
        tmp[tmp_len++] = static_cast<char>('0' + (value % 10));
        value /= 10;
      }
    }

    for (size_t i = 0; i < tmp_len; ++i)
    {
      AppendChar(dst, size, pos, tmp[tmp_len - 1 - i]);
    }
  }

  void FinalizeBuffer(char *dst, const size_t size, const size_t pos)
  {
    if (size == 0)
    {
      return;
    }

    const size_t write_pos = (pos < size) ? pos : (size - 1);
    dst[write_pos] = '\0';
  }

  void BuildSuiteLine(char *out, const size_t size, const char *suite_name, const int failures)
  {
    if (!out || size == 0)
    {
      return;
    }

    size_t pos = 0;
    const char *name = suite_name ? suite_name : "";

    for (size_t i = 0; i < 20 && name[i] != '\0'; ++i)
    {
      AppendChar(out, size, pos, name[i]);
    }

    if (failures)
    {
      AppendStr(out, size, pos, " : ");
      AppendUnsigned(out, size, pos, static_cast<unsigned int>(failures));
      AppendStr(out, size, pos, " failures");
    }
    else
    {
      AppendStr(out, size, pos, " SUCCESS !");
    }

    FinalizeBuffer(out, size, pos);
  }

  void BuildStatsLine(char *out, const size_t size, const unsigned int tests,
                      const unsigned int assertions, const unsigned int failures)
  {
    if (!out || size == 0)
    {
      return;
    }

    size_t pos = 0;
    AppendUnsigned(out, size, pos, tests);
    AppendStr(out, size, pos, " tests, ");
    AppendUnsigned(out, size, pos, assertions);
    AppendStr(out, size, pos, " assertions, ");
    AppendUnsigned(out, size, pos, failures);
    AppendStr(out, size, pos, " failures");
    FinalizeBuffer(out, size, pos);
  }

  void PushResultLine(const char *text)
  {
    ResultLine line = {};
    size_t pos = 0;
    AppendStr(line.text, kBufferSize, pos, text ? text : "");
    FinalizeBuffer(line.text, kBufferSize, pos);
    g_results.push_back(line);
  }

  void RenderResults(const size_t start_index)
  {
    char line_buffer[kDisplayColumns + 1];
    for (uint8_t i = 0; i < kDisplayLines; ++i)
    {
      const size_t line_index = start_index + i;
      const char *src = (line_index < g_results.size()) ? g_results[line_index].text : "";
      for (uint8_t col = 0; col < kDisplayColumns; ++col)
      {
        line_buffer[col] = ' ';
      }

      uint8_t col = 0;
      while (src[col] != '\0' && col < kDisplayColumns)
      {
        line_buffer[col] = src[col];
        ++col;
      }

      line_buffer[kDisplayColumns] = '\0';
      SRL::ASCII::Print(line_buffer, 0, i);
    }
  }

  void UpdateDisplay(size_t &start_index)
  {
    if (g_results.size() > kDisplayLines)
    {
      start_index = g_results.size() - kDisplayLines;
    }

    RenderResults(start_index);
    SRL::Core::Synchronize();
  }
} // namespace

extern "C"
{
  const uint8_t buffer_size = 255;
  char buffer[buffer_size] = {};
}

// Define a macro to capture test suite results
#define MU_DISPLAY_SATURN(suite_name)                 \
  BuildSuiteLine(results_buffer, kBufferSize,         \
                 #suite_name, suite_error_counter);   \
  PushResultLine(results_buffer);

#define RUN_AND_DISPLAY_SUITE(suite) \
  MU_RUN_SUITE(suite);               \
  MU_DISPLAY_SATURN(suite);          \
  UpdateDisplay(start_index);
