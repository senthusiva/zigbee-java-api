package com.zigbee.api;

import java.util.ArrayList;
import java.util.Arrays;

import org.apache.commons.lang3.StringUtils;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;

public class SensorConnection {
	public static String[] sdata = null; // sensor data
	private SerialPort comPort;

	/**
	 * Connects to COM-Port and receives sensor data
	 * 
	 * @param portDescriptor
	 */
	public void sensorConnection(String portDescriptor) {
		comPort = SerialPort.getCommPort(portDescriptor);
		comPort.setComPortParameters(38400, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
		comPort.openPort();
		comPort.addDataListener(new SerialPortDataListener() {
			public int getListeningEvents() {
				return SerialPort.LISTENING_EVENT_DATA_RECEIVED;
			}

			public void serialEvent(SerialPortEvent event) {
				byte[] newData = event.getReceivedData();
				// Tested with SHT21 Sensor with following ID Implementation in AtmelStudio:
				// uint8_t temp_output[]="xxx.xx_DEG_SHT1:1_EG1)";
				// uint8_t hmd_output[]= "xx.xx_HMD_SHT1:1_EG1)";
				String raw_data = " ";
				for (int i = 0; i < newData.length; ++i) {
					raw_data = raw_data.concat(Character.toString((char) newData[i]));
					if (Character.toString((char) newData[i]).contains(")")) {
						sdata = cleanData(sdata = StringUtils.substringsBetween(raw_data, " ", ")"));

						// check (for SHT21), if sdata-array contains five values and
						// if sensordata value is bigger than 5: xx.xx
						if (sdata.length == 5 && sdata[0].length() >= 5 && !(sdata[0].contains("x"))) {
							System.out.println(Arrays.toString(sdata));
						}

						// check if it's battery data
						// battery percentage value must be greater than 3, e.g.: 001, 010, 100,...
						if (sdata.length == 4 && sdata[1].contains("BAT") && sdata[0].length() >= 3
								&& !(sdata[0].contains("x"))) {
							System.out.println(Arrays.toString(sdata));
						}
						break;
					}
				}
			}
		});
	}

	/**
	 * close port
	 */
	public void closePort() {
		if (comPort != null)
			comPort.closePort();
	}

	/**
	 * takes the data and cleans it.
	 * 
	 * @param sdata
	 * @return
	 */
	public static String[] cleanData(String[] sdata) {
		sdata[0] = StringUtils.join(sdata[0], "_", System.currentTimeMillis());
		sdata[0] = StringUtils.normalizeSpace(sdata[0]);
		if (Character.valueOf(sdata[0].charAt(0)).equals('+'))
			sdata[0] = new StringBuilder(sdata[0]).deleteCharAt(0).toString();
		// SHT21 Sensor data before splitting:
		// xxx.xx_DEG_SHT_EG1_yyyy-mm-ddhh:mm:ss:ns
		sdata = sdata[0].split("_");

		return sdata;
	}

	/**
	 * returns the list of available ports
	 * 
	 * @return
	 */
	public static String[] getPortNames() {
		SerialPort[] ports = SerialPort.getCommPorts();
		ArrayList<String> portNames = new ArrayList<String>();
		for (SerialPort port : ports) {
			portNames.add(port.getSystemPortName());
		}
		return portNames.toArray(new String[] {});
	}

	public static void main(String[] args) {
		SensorConnection sn = new SensorConnection();
		sn.sensorConnection("COM3");
	}
}
