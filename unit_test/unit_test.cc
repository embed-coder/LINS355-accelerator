#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include "../LINS355.h"
#include "../m2m_csv.h"

#define DEVICE_FILE_1 "/dev/ttyUSB0"
#define DEVICE_FILE_2 "/dev/ttyUSB1"
#define DEVICE_FILE_3 "/dev/ttyUSB2"
#define DATA_FILE "data.csv"

LINS355Data *lins355_data;
void read_from_device(LINS355 *device, u_int16_t interval_ms)
{
    lins355_data = device->ReadData();
    if (lins355_data)
    {
        std::cout << "Timestamp: " << lins355_data->timestamp << std::endl;
        std::cout << "Accel x: " << lins355_data->data.at(0) << std::endl;
        std::cout << "Accel y: " << lins355_data->data.at(1) << std::endl;
        std::cout << "Accel z: " << lins355_data->data.at(2) << std::endl;
    }
}

/**
 * @brief Test OK for all the cases related to LINS355 device
 *  Test env: loop connection between /dev/ttyUSB0 and /dev/ttyUSB1
 * The data will be send from /dev/ttyUSB1 to /dev/ttyUSB0, the received data on /dev/ttyUSB0 will be asserted in the test cases
 */
TEST(LINS355_Device, OK)
{
    LINS355 *lins355_test = new LINS355(DEVICE_FILE_1, LibSerial::BaudRate::BAUD_115200, 100);

    // Expect port is openned
    EXPECT_EQ(lins355_test->IsOpen(), true);

    // Read data
    std::thread read_thread(read_from_device, lins355_test, 1);
    system("bash test_script.sh OK");
    read_thread.join();

    // Expect not null pointer returned
    EXPECT_NE(lins355_data, nullptr);

    // Expect valid read data
    EXPECT_FLOAT_EQ(lins355_data->data.at(0), 6.499023f); // Accel_X
    EXPECT_FLOAT_EQ(lins355_data->data.at(1), 2.343750f); // Accel_Y
    EXPECT_FLOAT_EQ(lins355_data->data.at(2), 3.203125f); // Accel_Z

    lins355_test->Close();

    // Expect port is closed
    EXPECT_EQ(lins355_test->IsOpen(), false);

    delete lins355_test;
    delete lins355_data;
}

/**
 * @brief Open fail on CRC error from data
 *  Test env: loop connection between /dev/ttyUSB0 and /dev/ttyUSB1
 * The data will be send from /dev/ttyUSB1 to /dev/ttyUSB0, the received data on /dev/ttyUSB0 will be asserted in the test cases
 */
TEST(LINS355_Device, FAIL_CRC_Error)
{
    LINS355 *lins355_test = new LINS355(DEVICE_FILE_1, LibSerial::BaudRate::BAUD_115200, 100);

    // Expect port is openned
    EXPECT_EQ(lins355_test->IsOpen(), true);

    // Read data
    std::thread read_thread(read_from_device, lins355_test, 1);
    system("bash test_script.sh FAIL_CRC");
    read_thread.join();

    // Expect null pointer returned because of CRC error
    EXPECT_EQ(lins355_data, nullptr);

    lins355_test->Close();

    // Expect port is closed
    EXPECT_EQ(lins355_test->IsOpen(), false);

    delete lins355_test;
    delete lins355_data;
}

#define DATA_FILE "data.csv"
/**
 * @brief Test creating, writing, reading data on csv file
 *  Test env: data will be write and read on data.csv
 */
TEST(M2M_CSV, OK)
{
    std::vector<std::string> columns{"Timestamp (UTC)", "Acc_x", "Acc_y", "Acc_z"};
    M2M_CSV *m2m_csv = new M2M_CSV(DATA_FILE, columns);
    LINS355Data data{
        .timestamp = "1655163581",
        .data = {
            6.49902f,
            2.34375f,
            3.20312f}};

    // Expect not null pointer returned
    EXPECT_NE(m2m_csv, nullptr);

    // Expect successful writing returned
    EXPECT_EQ(m2m_csv->Write(data), EXIT_SUCCESS);

    std::vector<LINS355Data> *read_data = m2m_csv->Read();

    // Expect valid read data
    EXPECT_EQ(read_data->at(0).timestamp, "1655163581");           // Timestamp
    EXPECT_FLOAT_EQ(read_data->at(0).data.at(0), data.data.at(0)); // Accel_X
    EXPECT_FLOAT_EQ(read_data->at(0).data.at(1), data.data.at(1)); // Accel_Y
    EXPECT_FLOAT_EQ(read_data->at(0).data.at(2), data.data.at(2)); // Accel_Z

    delete m2m_csv;
}

/**
 * @brief Test on a invalid column names in data file
 *  Test env: an invalid column name will be put into data.csv before running the test case
 */
TEST(M2M_CSV, FAIL_DataFile_Invalid_Column_Name)
{
    std::vector<std::string> columns{"Timestamp (UTC)", "Acc_x", "Acc_y", "Acc_z"};

    // Create an invalid colomn name data file
    std::string command = "bash test_script.sh FAIL_DataFile_Invalid_Column_Name ";
    command.append(DATA_FILE);
    system(command.c_str());

    M2M_CSV *m2m_csv;
    try
    {
        m2m_csv = new M2M_CSV(DATA_FILE, columns);
    }
    catch (...)
    {
        std::cout << "Test case: M2M_CSV.FAIL_DataFile_Invalid_Column_Name, as expected" << std::endl;
        if (m2m_csv)
            delete m2m_csv;
    }

    // Expect not null pointer returned
    EXPECT_EQ(m2m_csv, nullptr);

    // Delete the invalid file after the test case
    command = "sudo rm -f ";
    command.append(DATA_FILE);
    system(command.c_str());
}

/**
 * @brief Test on reading an non-existing data file
 *  Test env: the data file will be deleted before running the reading test case
 */
TEST(M2M_CSV, FAIL_DataFile_Non_Existing)
{
    std::vector<std::string> columns{"Timestamp (UTC)", "Acc_x", "Acc_y", "Acc_z"};
    std::vector<LINS355Data> *read_data;

    M2M_CSV *m2m_csv;
    try
    {
        m2m_csv = new M2M_CSV(DATA_FILE, columns);
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "Test case: M2M_CSV.FAIL_DataFile_Invalid_Column_Name, as expected" << std::endl;
        if (m2m_csv)
            delete m2m_csv;
    }

    // Expect not null pointer returned
    EXPECT_NE(m2m_csv, nullptr);

    // Delete the file before running the test case
    std::string command = "bash test_script.sh FAIL_DataFile_Non_Existing ";
    command.append(DATA_FILE);
    system(command.c_str());

    read_data = m2m_csv->Read();

    // Expect null pointer returned
    EXPECT_EQ(read_data, nullptr);

    delete m2m_csv;
}