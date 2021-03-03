package com.zigbee.api;

import org.apache.commons.lang3.StringUtils;
import org.junit.Assert;
import org.junit.Test;

/**
 * tests, if the API can deal with certain sensor data and if data cleaning
 * works as expected
 *
 */
class SensorConnectionTest {

	@Test
	void sensorOutputTest() {
		String raw_sensor_data_1 = "\\r\\n +04.56_DEG_SHT_EG1))EGjsdfjls   ";
		String raw_sensor_data_2 = "+Eg1  +04.56_DEG_SHT_EG1))EGjsdfjls  .. ";

		String[] cleanData = { "04.56", "DEG", "SHT", "EG1" };

		// for loop [without the last element] instead of assertArrayEquals,
		// becausse we don't want to test the timestamp
		for (int i = 0; i < 4; i++) {
			Assert.assertEquals(cleanData[i],
					SensorConnection.cleanData(StringUtils.substringsBetween(raw_sensor_data_1, " ", ")"))[i]);

			Assert.assertEquals(cleanData[i],
					SensorConnection.cleanData(StringUtils.substringsBetween(raw_sensor_data_2, " ", ")"))[i]);
		}
	}

	@Test
	void batteryOutputTest() {
		String raw_battery_data_1 = "\\r\\n 038_BAT_EG1))EG1))";
		String raw_battery_data_2 = "EG1)..     038_BAT_EG1))EG1))..";

		String[] cleanData = { "038", "BAT", "EG1" };

		// for loop [without the last element] instead of assertArrayEquals,
		// becausse we don't want to test the timestamp
		for (int i = 0; i < 3; i++) {
			Assert.assertEquals(cleanData[i],
					SensorConnection.cleanData(StringUtils.substringsBetween(raw_battery_data_1, " ", ")"))[i]);

			Assert.assertEquals(cleanData[i],
					SensorConnection.cleanData(StringUtils.substringsBetween(raw_battery_data_2, " ", ")"))[i]);
		}
	}

	@Test
	void dataArrayLengthTest() {
		String raw_sensor_data = "\\r\\n +04.56_DEG_SHT_EG1))EGjsdfjls   ";
		String raw_battery_data = "EG1)..     038_BAT_EG1))EG1))..";

		String[] cleanSensorData = { "04.56", "DEG", "SHT", "EG1", Long.toString(System.currentTimeMillis()) };
		String[] cleanBatteryData = { "038", "BAT", "EG1", Long.toString(System.currentTimeMillis()) };

		int actualSensorLength = SensorConnection
				.cleanData(StringUtils.substringsBetween(raw_sensor_data, " ", ")")).length;

		int actualBatteryLength = SensorConnection
				.cleanData(StringUtils.substringsBetween(raw_battery_data, " ", ")")).length;

		Assert.assertTrue(cleanSensorData.length == actualSensorLength);
		Assert.assertEquals(cleanBatteryData.length, actualBatteryLength);
	}

	@Test
	void getComPortTest() {
		String[] expectedCommPort = { "COM3" };
		String[] actualCommPort = SensorConnection.getPortNames();

		Assert.assertArrayEquals(expectedCommPort, actualCommPort);
		Assert.assertEquals(expectedCommPort.length, actualCommPort.length);
	}
}
