dist_doc_DATA = README.md

AM_DEFAULT_SOURCE_EXT = .cc

lib_LIBRARIES = google/protobuf/libprotobuf.a whisperlib/libwhisperlib.a

google_protobuf_libprotobuf_a_SOURCES = \
  google/protobuf/descriptor.cc \
  google/protobuf/descriptor.pb.cc \
  google/protobuf/descriptor_database.cc \
  google/protobuf/dynamic_message.cc \
  google/protobuf/extension_set.cc \
  google/protobuf/extension_set_heavy.cc \
  google/protobuf/generated_message_reflection.cc \
  google/protobuf/generated_message_util.cc \
  google/protobuf/message.cc \
  google/protobuf/message_lite.cc \
  google/protobuf/reflection_ops.cc \
  google/protobuf/repeated_field.cc \
  google/protobuf/service.cc \
  google/protobuf/text_format.cc \
  google/protobuf/unknown_field_set.cc \
  google/protobuf/wire_format.cc \
  google/protobuf/wire_format_lite.cc \
  google/protobuf/io/coded_stream.cc \
  google/protobuf/io/gzip_stream.cc \
  google/protobuf/io/printer.cc \
  google/protobuf/io/tokenizer.cc \
  google/protobuf/io/zero_copy_stream.cc \
  google/protobuf/io/zero_copy_stream_impl.cc \
  google/protobuf/io/zero_copy_stream_impl_lite.cc \
  google/protobuf/stubs/common.cc \
  google/protobuf/stubs/once.cc \
  google/protobuf/stubs/structurally_valid.cc \
  google/protobuf/stubs/strutil.cc \
  google/protobuf/stubs/substitute.cc

if HAVE_ICU
EXTRA_whisperlib_libwhisperlib_a_SOURCES = \
  whisperlib/url/google-url/gurl.cc \
  whisperlib/url/google-url/url_canon_etc.cc \
  whisperlib/url/google-url/url_canon_fileurl.cc \
  whisperlib/url/google-url/url_canon_host.cc \
  whisperlib/url/google-url/url_canon_icu.cc \
  whisperlib/url/google-url/url_canon_internal.cc \
  whisperlib/url/google-url/url_canon_ip.cc \
  whisperlib/url/google-url/url_canon_path.cc \
  whisperlib/url/google-url/url_canon_pathurl.cc \
  whisperlib/url/google-url/url_canon_query.cc \
  whisperlib/url/google-url/url_canon_relative.cc \
  whisperlib/url/google-url/url_canon_stdurl.cc \
  whisperlib/url/google-url/url_parse.cc \
  whisperlib/url/google-url/url_parse_file.cc \
  whisperlib/url/google-url/url_util.cc
endif

whisperlib_libwhisperlib_a_SOURCES = \
  $(EXTRA_whisperlib_libwhisperlib_a_SOURCES) \
  whisperlib/base/app.cc \
  whisperlib/base/date.cc \
  whisperlib/base/errno.cc \
  whisperlib/base/re.cc \
  whisperlib/base/signal_handlers.cc \
  whisperlib/base/stringprintf.cc \
  whisperlib/base/strutil.cc \
  whisperlib/base/strutil_format.cc \
  whisperlib/base/system.cc \
  whisperlib/base/timer.cc \
  whisperlib/base/util.cc \
  whisperlib/http/failsafe_http_client.cc \
  whisperlib/http/http_client_protocol.cc \
  whisperlib/http/http_consts.cc \
  whisperlib/http/http_header.cc \
  whisperlib/http/http_request.cc \
  whisperlib/http/http_server_protocol.cc \
  whisperlib/io/buffer/data_block.cc \
  whisperlib/io/buffer/memory_stream.cc \
  whisperlib/io/checkpoint/checkpoint_tool.cc \
  whisperlib/io/checkpoint/checkpointing.cc \
  whisperlib/io/checkpoint/state_keeper.cc \
  whisperlib/io/checkpoint/state_keeper_tool.cc \
  whisperlib/io/file/aio_file.cc \
  whisperlib/io/file/buffer_manager.cc \
  whisperlib/io/file/fd.cc \
  whisperlib/io/file/fd_input_stream.cc \
  whisperlib/io/file/file.cc \
  whisperlib/io/file/file_input_stream.cc \
  whisperlib/io/file/file_output_stream.cc \
  whisperlib/io/ioutil.cc \
  whisperlib/io/logio/logio.cc \
  whisperlib/io/logio/recordio.cc \
  whisperlib/io/output_stream.cc \
  whisperlib/io/stream_base.cc \
  whisperlib/io/util/base64.cc \
  whisperlib/io/util/sha256.cc \
  whisperlib/io/zlib/zlibwrapper.cc \
  whisperlib/net/address.cc \
  whisperlib/net/alarm.cc \
  whisperlib/net/connection.cc \
  whisperlib/net/dns_resolver.cc \
  whisperlib/net/ipclassifier.cc \
  whisperlib/net/selectable.cc \
  whisperlib/net/selectable_filereader.cc \
  whisperlib/net/selector.cc \
  whisperlib/net/selector_base.cc \
  whisperlib/net/timeouter.cc \
  whisperlib/net/udp_connection.cc \
  whisperlib/rpc/codec/rpc_json_decoder.cc \
  whisperlib/rpc/codec/rpc_json_encoder.cc \
  whisperlib/rpc/rpc_controller.cc \
  whisperlib/rpc/rpc_http_client.cc \
  whisperlib/rpc/rpc_http_server.cc \
  whisperlib/sync/event.cc \
  whisperlib/sync/thread.cc \
  whisperlib/sync/thread_pool.cc \
  whisperlib/url/simple_url.cc

nobase_include_HEADERS = \
  whisperlib/base/app.h \
  whisperlib/base/basictypes.h \
  whisperlib/base/cache.h \
  whisperlib/base/callback/callback.h \
  whisperlib/base/callback/callback1.h \
  whisperlib/base/callback/callback2.h \
  whisperlib/base/callback/callback3.h \
  whisperlib/base/callback/closure.h \
  whisperlib/base/callback/result_callback1.h \
  whisperlib/base/callback/result_callback2.h \
  whisperlib/base/callback/result_callback3.h \
  whisperlib/base/callback/result_closure.h \
  whisperlib/base/callback.h \
  whisperlib/base/date.h \
  whisperlib/base/errno.h \
  whisperlib/base/free_list.h \
  whisperlib/base/gflags.h \
  whisperlib/base/log.h \
  whisperlib/base/lru_cache.h \
  whisperlib/base/port.h \
  whisperlib/base/re.h \
  whisperlib/base/ref_counted.h \
  whisperlib/base/scoped_ptr.h \
  whisperlib/base/signal_handlers.h \
  whisperlib/base/stringprintf.h \
  whisperlib/base/strutil.h \
  whisperlib/base/strutil_format.h \
  whisperlib/base/system.h \
  whisperlib/base/timer.h \
  whisperlib/base/types.h \
  whisperlib/base/util.h \
  whisperlib/http/failsafe_http_client.h \
  whisperlib/http/http_client_protocol.h \
  whisperlib/http/http_consts.h \
  whisperlib/http/http_header.h \
  whisperlib/http/http_request.h \
  whisperlib/http/http_server_protocol.h \
  whisperlib/io/buffer/data_block.h \
  whisperlib/io/buffer/memory_stream.h \
  whisperlib/io/buffer/protobuf_stream.h \
  whisperlib/io/checkpoint/checkpointing.h \
  whisperlib/io/checkpoint/state_keeper.h \
  whisperlib/io/file/aio_file.h \
  whisperlib/io/file/buffer_manager.h \
  whisperlib/io/file/fd.h \
  whisperlib/io/file/fd_input_stream.h \
  whisperlib/io/file/file.h \
  whisperlib/io/file/file_input_stream.h \
  whisperlib/io/file/file_output_stream.h \
  whisperlib/io/input_stream.h \
  whisperlib/io/iomarker.h \
  whisperlib/io/ioutil.h \
  whisperlib/io/logio/logio.h \
  whisperlib/io/logio/recordio.h \
  whisperlib/io/num_streaming.h \
  whisperlib/io/output_stream.h \
  whisperlib/io/seeker.h \
  whisperlib/io/stream_base.h \
  whisperlib/io/util/base64.h \
  whisperlib/io/util/sha256.h \
  whisperlib/io/zlib/zlibwrapper.h \
  whisperlib/net/address.h \
  whisperlib/net/alarm.h \
  whisperlib/net/connection.h \
  whisperlib/net/dns_resolver.h \
  whisperlib/net/ipclassifier.h \
  whisperlib/net/selectable.h \
  whisperlib/net/selectable_filereader.h \
  whisperlib/net/selector.h \
  whisperlib/net/selector_base.h \
  whisperlib/net/timeouter.h \
  whisperlib/net/udp_connection.h \
  whisperlib/net/user_authenticator.h \
  whisperlib/rpc/codec/rpc_decode_result.h \
  whisperlib/rpc/codec/rpc_decoder.h \
  whisperlib/rpc/codec/rpc_encoder.h \
  whisperlib/rpc/codec/rpc_json_decoder.h \
  whisperlib/rpc/codec/rpc_json_encoder.h \
  whisperlib/rpc/rpc_consts.h \
  whisperlib/rpc/rpc_controller.h \
  whisperlib/rpc/rpc_http_client.h \
  whisperlib/rpc/rpc_http_server.h \
  whisperlib/sync/event.h \
  whisperlib/sync/mutex.h \
  whisperlib/sync/producer_consumer_queue.h \
  whisperlib/sync/thread.h \
  whisperlib/sync/thread_pool.h \
  whisperlib/url/simple_url.h \
  whisperlib/url/url.h

if HAVE_GLOG
glog_check_programs = \
  whisperlib/io/logio/test/logio_test \
  whisperlib/io/logio/test/recordio_test

if HAVE_ICU
glog_icu_check_programs = \
  whisperlib/url/test/url_test
endif

endif

TEST_EXTENSIONS = .sh

standalone_test_programs = \
  whisperlib/base/test/lru_cache_test \
  whisperlib/base/test/strutil_test \
  whisperlib/http/test/http_header_test \
  whisperlib/io/buffer/test/data_block_test \
  whisperlib/io/buffer/test/memory_stream_test \
  whisperlib/io/checkpoint/test/checkpointing_test \
  whisperlib/io/checkpoint/test/state_keeper_test \
  whisperlib/net/test/address_test \
  whisperlib/net/test/dns_resolver_test \
  whisperlib/net/test/selector_test \
  whisperlib/net/test/udp_connection_test \
  $(glog_check_programs) \
  $(glog_icu_check_programs)

driver_check_programs = \
  whisperlib/http/test/failsafe_test \
  whisperlib/http/test/http_request_test \
  whisperlib/net/test/selectable_filereader_test

driver_test_scripts = \
  whisperlib/http/test/test_failsafe.sh \
  whisperlib/http/test/http_request_test.sh \
  whisperlib/net/test/selectable_filereader_test.sh


check_PROGRAMS = \
  $(standalone_test_programs) \
  $(driver_check_programs)

LDADD = whisperlib/libwhisperlib.a $(ICU_LIBS)

TESTS = $(standalone_test_programs) $(driver_test_scripts)
