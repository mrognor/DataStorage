project(DataStorage)

set (Headers DataStorage.h DataStorageRecordSet.h DataStorageRecord.h DataContainer.h DataSaver.h SmartPointerWrapper.h CsvParser.h)
set (Sources main.cpp DataStorage.cpp DataStorageRecordSet.cpp DataStorageRecord.cpp DataSaver.cpp CsvParser.cpp)

add_executable(DataStorage ${Headers} ${Sources})