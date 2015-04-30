#include "compute/edflib.h"
#include "compute/eeg_spectrogram.h"
#include "json11/json11.hpp"
#include "wslib/server_ws.hpp"

using namespace std;
using namespace SimpleWeb;
using namespace json11;

#define NUM_THREADS 4
#define PORT 8080
#define TEXT_OPCODE 129
#define BINARY_OPCODE 130

void send_message(SocketServer<WS> server, shared_ptr<SocketServer<WS>::Connection> connection, char* filename, std::string type, Json content, float * data)
{
  std::string header = content.dump();


  // append enough spaces so that the payload starts at an 8-byte
  // aligned position. The first four bytes will be the length of
  // the header, encoded as a 32 bit signed integer:
  unsigned header_len = header.size();
  header.resize(header_len + (8 - ((header_len + 4) % 8)), ' ');

  stringstream data_ss;
  data_ss << header_len << header << data;

  //server.send is an asynchronous function
  server.send(connection, data_ss, [&filename, &data](const boost::system::error_code & ec)
  {
    if (ec)
    {
      cout << "Server: Error sending message. " <<
           //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
           "Error: " << ec << ", error message: " << ec.message() << endl;
    }
    cleanup_spectrogram(filename, data);
    delete[] filename;
  }, BINARY_OPCODE);
}

void on_file_spectrogram(SocketServer<WS> server, shared_ptr<SocketServer<WS>::Connection> connection, Json data)
{
  std::string filename = data["filename"].string_value();
  float duration = data["duration"].number_value();

  spec_params_t spec_params;
  char *filename_c = new char[filename.length() + 1];
  strcpy(filename_c, filename.c_str());
  get_eeg_spectrogram_params(&spec_params, filename_c, duration);
  float* spec = (float*) malloc(sizeof(float) * spec_params.nblocks * spec_params.nfreqs);
  print_spec_params_t(&spec_params);
  eeg_spectrogram_handler(&spec_params, LL, spec);
  Json content = Json::object
  {
    {"action", "update"},
    {"nblocks", spec_params.nblocks},
    {"nfreqs", spec_params.nfreqs},
    {"canvasId", LL}
  };
  send_message(server, connection, filename_c, "spectrogram", content, spec);
}

void receive_message(SocketServer<WS> server, shared_ptr<SocketServer<WS>::Connection> connection, std::string type, Json content)
{
  if (type == "request_file_spectrogram")
  {
    on_file_spectrogram(server, connection, content);
  }
  else if (type == "information")
  {
    printf("%s", content.string_value());
  }
  else
  {
    printf("Unkown type: %s and content: %s", type, content.string_value());
  }
}

int main()
{
  //WebSocket (WS)-server at PORT using NUM_THREADS threads
  SocketServer<WS> server(PORT, NUM_THREADS);

  auto& spec_ws = server.endpoint["^/spectrogram/?$"];

  spec_ws.onopen = [](auto connection)
  {
    cout << "WebSocket opened" << endl;
  };

//C++14, lambda parameters declared with auto
//For C++11 use: (shared_ptr<SocketServer<WS>::Connection> connection, shared_ptr<SocketServer<WS>::Message> message)
  spec_ws.onmessage = [&server](auto connection, auto message)
  {
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

      receive_message(server, connection, type, content);
    };


//See RFC 6455 7.4.1. for status codes
    spec_ws.onclose = [](auto connection, int status, const string & reason)
    {
      cout << "WebSocket closed" << endl;
    };

//See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    spec_ws.onerror = [](auto connection, const boost::system::error_code & ec)
    {
      cout << "Error: " << ec << ", error message: " << ec.message() << endl;
    };


    thread server_thread([&server]()
    {
      //Start WS-server
      server.start();
    });

    server_thread.join();

    return 0;
  }
