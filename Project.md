You are supposed to develop a distributed chat/file-sharing application using BSD sockets. The application must comply with the following requirements.

    a) When a client joins the chat, details about all the files in the client’s shared directory must be shared with the community.

    b) Any member of the community should be able to search and download any shared file.

    c) Any member in the community must be able to do a full-duplex chat with any other member.

    d) There should be at least 4 members in the community.

You can use any of the hybrid/P2P models to complete these tasks.

Methods:
For running code on single machine we can just duplicate file in the given directory.
But for communication between different machines, need to somehow send file
Initially thought of using UDP, but as it does not guarentee packet order and trasmission, may mess up file
First when the serer acknowledges the user, it will save the user's IP and port number.
Then the server will close its connection.

Then if the client wants to get files, it will send server a message, the server will respond by sending all files it has
OR
The client sends the filename, then IF file is present server tells that yes and returns clients port and IP, else will return FileNotFound