import socket

HOST = "0.0.0.0"
PORT = 19090

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen()

    print(f"Listening on {HOST}:{PORT}")

    while True:
        connection, address = server.accept()
        print(f"Connected: {address}")

        with connection:
            buffer = ""

            while True:
                data = connection.recv(4096)

                if not data:
                    print(f"Disconnected: {address}")
                    break

                buffer += data.decode("utf-8")

                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    print(f"Vehicle frame: {line}")
