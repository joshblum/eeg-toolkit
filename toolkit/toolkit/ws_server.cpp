#include <armadillo>
#include <thread>
#include <algorithm>

#include "wslib/server_ws.hpp"
#include "config.hpp"
#include "helpers.hpp"
#include "json11/json11.hpp"
#include "storage/backends.hpp"
#include "compute/eeg_spectrogram.hpp"
#include "compute/eeg_change_point.hpp"

using namespace arma;
using namespace std;
using namespace json11;

#define BINARY_OPCODE 130

typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;

void send_message(WsServer* server, shared_ptr<WsServer::Connection> connection,
                  string msg_type, Json content, float* data, size_t data_size)
{
  unsigned long long start = getticks();
  Json msg = Json::object
  {
    {"type", msg_type},
    {"content", content}
  };
  string header = msg.dump();

  uint32_t header_len = get_byte_aligned_length(header);

  // append enough spaces so that the payload starts at an 8-byte
  // aligned position. The first four bytes will be the length of
  // the header, encoded as a 32 bit signed integer:
  header.resize(header_len, ' ');

  auto send_stream = make_shared<WsServer::SendStream>();
  send_stream->write((char*) &header_len, sizeof(uint32_t));
  send_stream->write(header.c_str(), header_len);
  if (data != NULL)
  {
    send_stream->write((char*) data, data_size);
  }

  string action = content["action"].string_value();
  // server.send is an asynchronous function
  server->send(connection, send_stream, [action, start](const boost::system::error_code & ec)
  {
    log_time_diff("send_message::" + action, start);
    if (ec)
    {
      cout << "Server: Error sending message. " <<
           // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html
           // Error Codes for error code meanings
           "Error: " << ec << ", error message: " << ec.message() << endl;
    }
  }, BINARY_OPCODE);
}

void log_json(Json content)
{
  cout << "Sending content " << content.dump() << endl;
}

void send_frowvec(WsServer* server,
                  shared_ptr<WsServer::Connection> connection,
                  string canvasId, string type,
                  frowvec vector)
{

  Json content = Json::object
  {
    {"action", "change_points"},
    {"type", type},
    {"canvasId", canvasId}
  };
  log_json(content);
  send_message(server, connection, "spectrogram", content, vector.memptr(), vector.n_elem);
}

void send_spectrogram_new(WsServer* server,
                          shared_ptr<WsServer::Connection> connection,
                          SpecParams spec_params, string canvasId)
{
  Json content = Json::object
  {
    {"action", "new"},
    {"nblocks", spec_params.nblocks},
    {"nfreqs", spec_params.nfreqs},
    {"fs", spec_params.fs},
    {"startTime", spec_params.start_time},
    {"endTime", spec_params.end_time},
    {"canvasId", canvasId}
  };
  log_json(content);
  send_message(server, connection, "spectrogram", content, NULL, -1);
}

void send_spectrogram_update(WsServer* server,
                             shared_ptr<WsServer::Connection> connection,
                             SpecParams spec_params, string canvasId,
                             fmat& spec_mat)
{

  Json content = Json::object
  {
    {"action", "update"},
    {"nblocks", spec_params.nblocks},
    {"nfreqs", spec_params.nfreqs},
    {"canvasId", canvasId}
  };
  size_t data_size = sizeof(float) * spec_mat.n_elem;
  log_json(content);
  send_message(server, connection, "spectrogram", content, spec_mat.memptr(), data_size);
}

void send_change_points(WsServer* server,
                        shared_ptr<WsServer::Connection> connection,
                        string canvasId,
                        cp_data_t* cp_data)
{
  send_frowvec(server, connection, canvasId, "change_points", cp_data->cp);
  send_frowvec(server, connection, canvasId, "summed_signal", cp_data->m);
}


void on_file_spectrogram(WsServer* server, shared_ptr<WsServer::Connection> connection, Json data)
{
  // TODO(joshblum): add data validation
  string mrn = data["mrn"].string_value();
  float start_time = data["startTime"].number_value();
  float end_time = data["endTime"].number_value();
  int ch = data["channel"].number_value();
  string ch_name = CH_NAME_MAP[ch];

  StorageBackend backend; // perhaps this should be a global thing..
  SpecParams spec_params = SpecParams(&backend, mrn, start_time, end_time);
  spec_params.print();
  cout << endl; // print newline between each spectrogram computation

  send_spectrogram_new(server, connection, spec_params, ch_name);

  fmat spec_mat = fmat(spec_params.nfreqs, spec_params.nblocks);
  string cached_mrn_name = backend.mrn_to_cached_mrn_name(mrn, ch_name);

  unsigned long long start = getticks();
  if (backend.array_exists(cached_mrn_name))
  {
    cout << "Using cached visualization!" << endl;
    backend.open_array(cached_mrn_name);
    backend.read_array(cached_mrn_name, spec_params.spec_start_offset, spec_params.spec_end_offset, spec_mat);
    backend.close_array(cached_mrn_name);
  } else {
    eeg_spectrogram(&spec_params, ch, spec_mat);
  }
  log_time_diff("eeg_spectrogram", start);

  send_spectrogram_update(server, connection, spec_params, ch_name, spec_mat);

  // cp_data_t cp_data;
  // get_change_points(spec_mat, &cp_data);
  // send_change_points(server, connection, ch_name, &cp_data);

  backend.close_array(mrn);
}

void receive_message(WsServer* server, shared_ptr<WsServer::Connection> connection, string type, Json content)
{
  if (type == "request_file_spectrogram")
  {
    on_file_spectrogram(server, connection, content);
  }
  else if (type == "information")
  {
    cout << content.string_value() << endl;
  }
  else
  {
    cout << "Unknown type: " << type << " and content: " << content.string_value() << endl;
  }
}

int main(int argc, char* argv[])
{
  int port;
  if (argc == 2)
  {
    port = atoi(argv[1]);
  }
  else
  {
    port = WS_DEFAULT_PORT;
  }

  // WebSocket (WS)-server at port using WS_NUM_THREADS threads
  int num_threads = max((int) thread::hardware_concurrency() - 1, 1);
  WsServer server(port, num_threads);

  auto& ws = server.endpoint["^/compute/spectrogram/?$"];

  ws.onmessage = [&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message)
  {
    auto message_str = message->string();
    string err;
    Json json = Json::parse(message_str, err);

    // TODO add error checking for null fields
    string type = json["type"].string_value();
    Json content = json["content"];

    receive_message(&server, connection, type, content);
  };

  ws.onopen = [](shared_ptr<WsServer::Connection> connection)
  {
    cout << "WebSocket opened" << endl;
  };


  // See RFC 6455 7.4.1. for status codes
  ws.onclose = [](shared_ptr<WsServer::Connection> connection, int status, const string & reason)
  {
    cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
  };

  // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  ws.onerror = [](shared_ptr<WsServer::Connection> connection, const boost::system::error_code & ec)
  {
    cout << "Server: Error in connection " << (size_t)connection.get() << ". " <<
         "Error: " << ec << ", error message: " << ec.message() << endl;
  };


  thread server_thread([&server, port, num_threads]()
  {
    cout << "WebSocket Server started at port: " << port << " using " << num_threads << " threads and backend: " << TOSTRING(BACKEND) << endl;
    // Start WS-server
    server.start();
  });

  server_thread.join();

  return 0;
}

