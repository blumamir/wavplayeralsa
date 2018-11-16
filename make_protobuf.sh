protoc -I=src/ --cpp_out=./src/generated/ --python_out=./src/generated/ --java_out=./src/generated ./src/*.proto
cp ./src/generated/player_command_pb2.py python-tools