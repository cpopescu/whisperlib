dist_doc_DATA = README.md

AM_DEFAULT_SOURCE_EXT = .cc

lib_LIBRARIES = whisperlib/libwhisperlib.a

LDADD = whisperlib/libwhisperlib.a $(ICU_LIBS)

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

extra_whisperlib_libwhisperlib_a_headers = \
  whisperlib/url/google-url/gurl.h \
  whisperlib/url/google-url/url_canon.h \
  whisperlib/url/google-url/url_canon_icu.h \
  whisperlib/url/google-url/url_canon_internal.h \
  whisperlib/url/google-url/url_canon_internal_file.h \
  whisperlib/url/google-url/url_canon_stdstring.h \
  whisperlib/url/google-url/url_file.h \
  whisperlib/url/google-url/url_parse.h \
  whisperlib/url/google-url/url_parse_internal.h \
  whisperlib/url/google-url/url_util.h

endif

if HAVE_PROTOBUF
rpc_protobuf_sources = \
  whisperlib/rpc/rpc_http_client.cc \
  whisperlib/rpc/rpc_http_server.cc \
  whisperlib/rpc/rpc_controller.cc

rpc_protobuf_headers = \
  whisperlib/rpc/rpc_consts.h \
  whisperlib/rpc/rpc_controller.h \
  whisperlib/rpc/rpc_http_client.h \
  whisperlib/rpc/rpc_http_server.h

endif


whisperlib_libwhisperlib_a_SOURCES = \
  $(EXTRA_whisperlib_libwhisperlib_a_SOURCES) \
  $(rpc_protobuf_sources) \
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
  whisperlib/sync/event.cc \
  whisperlib/sync/thread.cc \
  whisperlib/sync/thread_pool.cc \
  whisperlib/url/simple_url.cc

nobase_include_HEADERS = \
  whisperlib/base/core_config.h \
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
  whisperlib/base/core_errno.h \
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
  whisperlib/sync/event.h \
  whisperlib/sync/mutex.h \
  whisperlib/sync/producer_consumer_queue.h \
  whisperlib/sync/thread.h \
  whisperlib/sync/thread_pool.h \
  whisperlib/url/simple_url.h \
  whisperlib/url/url.h \
  $(extra_whisperlib_libwhisperlib_a_headers) \
  $(rpc_protobuf_headers)

if HAVE_GLOG
glog_check_programs = \
  whisperlib/io/logio/test/logio_test \
  whisperlib/io/logio/test/recordio_test

if HAVE_ICU
glog_icu_check_programs = \
  whisperlib/url/test/url_test
endif

if HAVE_PROTOBUF
rpc_test_programs = \
  whisperlib/rpc/test/rpc_test_client \
  whisperlib/rpc/test/rpc_test_server
endif

whisperlib_rpc_test_rpc_test_client_LDADD = whisperlib/rpc/test/rpc_test_proto.pb.o $(LDADD)
whisperlib_rpc_test_rpc_test_server_LDADD = whisperlib/rpc/test/rpc_test_proto.pb.o $(LDADD)

whisperlib/rpc/test/rpc_test_proto.pb.cc: whisperlib/rpc/test/rpc_test_proto.proto
	protoc $< --cpp_out=.

endif

TEST_EXTENSIONS = .sh

standalone_test_programs = \
  whisperlib/base/test/lru_cache_test \
  whisperlib/base/test/strutil_test \
  whisperlib/http/test/http_header_test \
  whisperlib/http/test/http_server_test \
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
  $(driver_check_programs) \
  $(rpc_test_programs)

TESTS = $(standalone_test_programs) $(driver_test_scripts)

clean-local:
	-rm -f whisperlib/rpc/test/rpc_test_proto.pb.{h,cc}
	-rm -rf autom4te.cache
	-rm -f stamp-h1
