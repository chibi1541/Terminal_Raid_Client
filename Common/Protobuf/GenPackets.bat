pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Protocol.proto
protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto

GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=S_ --send=C_

IF ERRORLEVEL 1 PAUSE

XCOPY Enum.pb.h "../../GameProj/source/Protocol" /E /Y /I
XCOPY Enum.pb.cc "../../GameProj/source/Protocol" /E /Y /I
XCOPY Struct.pb.h "../../GameProj/source/Protocol" /E /Y /I
XCOPY Struct.pb.cc "../../GameProj/source/Protocol" /E /Y /I
XCOPY Protocol.pb.h "../../GameProj/source/Protocol" /E /Y /I
XCOPY Protocol.pb.cc "../../GameProj/source/Protocol" /E /Y /I
XCOPY ServerPacketHandler.h "../../GameProj/source/Protocol" /E /Y /I


DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE