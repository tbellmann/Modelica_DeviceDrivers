within Modelica_DeviceDrivers.InputDevices;
package SpaceMouse "A driver accessing the 3DConnexion SpaceMouse."
extends Modelica_DeviceDrivers.Utilities.Icons.DriverIcon;
function getData "reads data from 3dConnexion SpaceMouse"
output Real Axes[6] "Axes values";
output Integer Buttons[16] "Buttons values";
external "C" MDD_spaceMouseGetData(Axes, Buttons)
annotation(Include = "#include \"MDDSpaceMouse.h\" ",
           Library = {"MDDSpaceMouse", "X11"});
annotation(__OpenModelica_Impure=true, __iti_Impure=true);
end getData;
end SpaceMouse;
