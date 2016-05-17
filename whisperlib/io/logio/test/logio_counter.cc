// Counts records in a log.
//
#include "whisperlib/base/types.h"
#include "whisperlib/base/log.h"
#include "whisperlib/base/timer.h"
#include "whisperlib/base/system.h"
#include "whisperlib/sync/thread.h"
#include "whisperlib/base/gflags.h"

#include "whisperlib/io/logio/logio.h"

DEFINE_string(dir, "", "Logs directory");
DEFINE_string(name, "", "Logs name");

int main(int argc, char* argv[]) {
  whisper::common::Init(argc, argv);
  whisper::io::LogReader reader(FLAGS_dir, FLAGS_name);
  printf("%" PRId64 "\n", whisper::io::CountLogRecords(&reader));
  return 0;
}
