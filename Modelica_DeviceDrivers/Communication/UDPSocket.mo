within Modelica_DeviceDrivers.Communication;
class UDPSocket "A driver for UDP packet network communication."
extends ExternalObject;
encapsulated function constructor
    "creates an UDPSocket instance with a given listening port."
    import Modelica_DeviceDrivers.Communication.UDPSocket;
  input Integer port "listening port";
  input Integer bufferSize=16*1024 "Size of receive buffer";
  input Boolean useNonBlockingRecv
      "if true, the simulation does not wait for incoming packages";
  output UDPSocket socket;
external "C" socket = MDD_udpConstructor(port,bufferSize,useNonBlockingRecv);
annotation(IncludeDirectory="modelica://Modelica_DeviceDrivers/Resources/Include",
           Include = "#include \"MDDUDPSocket.h\" ");
end constructor;

encapsulated function destructor
    import Modelica_DeviceDrivers.Communication.UDPSocket;
  input UDPSocket socket;
external "C" MDD_udpDestructor(socket);
annotation(IncludeDirectory="modelica://Modelica_DeviceDrivers/Resources/Include",
           Include = "#include \"MDDUDPSocket.h\" ");
end destructor;
end UDPSocket;
