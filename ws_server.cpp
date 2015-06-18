#include <armadillo>

#include "compute/edflib.h"
#include "compute/eeg_spectrogram.hpp"
#include "compute/eeg_change_point.hpp"
#include "json11/json11.hpp"
#include "wslib/server_ws.hpp"

using namespace arma;
using namespace std;
using namespace SimpleWeb;
using namespace json11;

#define NUM_THREADS 4
#define PORT 8080
#define TEXT_OPCODE 129
#define BINARY_OPCODE 130

const char* CH_NAME_MAP[] = {"LL", "LP", "RP", "RL"};

void send_message(SocketServer<WS>* server, shared_ptr<SocketServer<WS>::Connection> connection,
                  std::string msg_type, Json content, float* data, size_t data_size)
{

  Json msg = Json::object
  {
    {"type", msg_type},
    {"content", content}
  };
  std::string header = msg.dump();

  uint32_t header_len = header.size() + (8 - ((header.size() + 4) % 8));
  // append enough spaces so that the payload starts at an 8-byte
  // aligned position. The first four bytes will be the length of
  // the header, encoded as a 32 bit signed integer:
  header.resize(header_len, ' ');

  stringstream data_ss;
  data_ss.write((char*) &header_len, sizeof(uint32_t));
  data_ss.write(header.c_str(), header_len);
  if (data != NULL)
  {
    data_ss.write((char*) data, data_size);
  }

  // server.send is an asynchronous function
  server->send(connection, data_ss, [](const boost::system::error_code & ec)
  {
    if (ec)
    {
      cout << "Server: Error sending message. " <<
           // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html
           // Error Codes for error code meanings
           "Error: " << ec << ", error message: " << ec.message() << endl;
    }
  }, BINARY_OPCODE);
}

void log_json(Json content) {
  cout << "Sending content " << content.dump() << endl;
}

void send_frowvec(SocketServer<WS>* server,
                  shared_ptr<SocketServer<WS>::Connection> connection,
                  std::string canvasId, std::string type,
                  frowvec* vector)
{

  Json content = Json::object
  {
    {"action", "change_points"},
    {"type", type},
    {"canvasId", canvasId}
  };
  log_json(content);
  send_message(server, connection, "spectrogram", content, vector->memptr(), vector->n_elem);
}

void send_spectrogram_new(SocketServer<WS>* server,
                          shared_ptr<SocketServer<WS>::Connection> connection,
                          spec_params_t spec_params, std::string canvasId)
{
  Json content = Json::object
  {
    {"action", "new"},
    {"nblocks", spec_params.nblocks},
    {"nfreqs", spec_params.nfreqs},
    {"fs", spec_params.fs},
    {"length", spec_params.spec_len},
    {"canvasId", canvasId}
  };
  log_json(content);
  send_message(server, connection, "spectrogram", content, NULL, -1);
}

void send_spectrogram_update(SocketServer<WS>* server,
                             shared_ptr<SocketServer<WS>::Connection> connection,
                             spec_params_t spec_params, std::string canvasId,
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
  float* spec_arr = (float*) malloc(data_size);
  serialize_spec_mat(&spec_params, spec_mat, spec_arr);
  log_json(content);
  send_message(server, connection, "spectrogram", content, spec_arr, data_size);
  free(spec_arr);
}

void send_change_points(SocketServer<WS>* server,
                        shared_ptr<SocketServer<WS>::Connection> connection,
                        std::string canvasId,
                        cp_data_t* cp_data)
{
  send_frowvec(server, connection, canvasId, "change_points", cp_data->cp);
  send_frowvec(server, connection, canvasId, "summed_signal", cp_data->m);
}

void on_file_spectrogram(SocketServer<WS>* server, shared_ptr<SocketServer<WS>::Connection> connection, Json data)
{
  std::string filename = data["filename"].string_value();
  float duration = data["duration"].number_value();

  spec_params_t spec_params;
  char *filename_c = new char[filename.length() + 1];
  strcpy(filename_c, filename.c_str());
  get_eeg_spectrogram_params(&spec_params, filename_c, duration);
  print_spec_params_t(&spec_params);
  const char* ch_name;
  for (int ch = 0; ch < NUM_CH; ch++)
  {
    ch_name = CH_NAME_MAP[ch];
    send_spectrogram_new(server, connection, spec_params, ch_name);
    fmat spec_mat = fmat(spec_params.nfreqs, spec_params.nblocks);
    eeg_spectrogram(&spec_params, ch, spec_mat);

    cp_data_t cp_data;
    get_change_points(spec_mat, &cp_data);

    send_spectrogram_update(server, connection, spec_params, ch_name, spec_mat);
    send_change_points(server, connection, ch_name, &cp_data);
    this_thread::sleep_for(chrono::seconds(5)); // TODO(joshblum): fix this..
  }
  close_edf(filename_c);
}

void receive_message(SocketServer<WS>* server, shared_ptr<SocketServer<WS>::Connection> connection, std::string type, Json content)
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

int main()
{
  //WebSocket (WS)-server at PORT using NUM_THREADS threads
  SocketServer<WS> server(PORT, NUM_THREADS);

  auto& ws = server.endpoint["^/compute/spectrogram/?$"];

//C++14, lambda parameters declared with auto
//For C++11 use: (shared_ptr<SocketServer<WS>::Connection> connection, shared_ptr<SocketServer<WS>::Message> message)
  ws.onmessage = [&server](auto connection, auto message)
  {
    //To receive message from client as string (data_ss.str())
    stringstream data_ss;
    message->data >> data_ss.rdbuf();
    std::string data_s, err;
    data_s = data_ss.str();
    Json json = Json::parse(data_s, err);
    // TODO add error checking for null fields
    std::string type = json["type"].string_value();
    Json content = json["content"];

    receive_message(&server, connection, type, content);
  };

  ws.onopen = [](auto connection)
  {
    cout << "WebSocket opened" << endl;
  };


  //See RFC 6455 7.4.1. for status codes
  ws.onclose = [](auto connection, int status, const string & reason)
  {
    cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
  };

  //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  ws.onerror = [](auto connection, const boost::system::error_code & ec)
  {
    cout << "Server: Error in connection " << (size_t)connection.get() << ". " <<
         "Error: " << ec << ", error message: " << ec.message() << endl;
  };


  thread server_thread([&server]()
  {
    cout << "WebSocket Server started at port: " << PORT << endl;
    //Start WS-server
    server.start();
  });

  server_thread.join();

  return 0;
}
