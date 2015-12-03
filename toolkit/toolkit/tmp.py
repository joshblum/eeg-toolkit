import socket
import os
try:
  os.unlink("hi")
except:
  pass

backlog = 5
size = 1024
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.bind("hi")
s.listen(backlog)
client, address = s.accept()
while 1:
  data = client.recv(size)
  print repr(data)
  if data:
    if "GETVAL" in data:
      client.send("1 results found\n")
      client.send("result :)\n")
    elif "LISTVAL" in data:
      client.send("5 results found\n")
      for i in range(5):
        client.send("result :) %d \n" % i)
    elif "FLUSH" in data:
      client.send("1 good\n")
