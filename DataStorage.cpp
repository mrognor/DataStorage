#include <iostream>
#include <unordered_map>
#include <cstring>
#include <list>
#include <typeinfo>
#include <functional>

// Class to make real time type check
class DataTypeSaver
{
private:
    // Type info struct to save type
    const std::type_info& TypeInfo;

public:
    // Constructor to set type variable
    DataTypeSaver(const std::type_info& type) : TypeInfo(type) {}

    // Type getter
    const std::type_info& GetDataType() { return TypeInfo; }
};

// Class to store data with any type
// If pointers are stored in the class object, then you can set a custom data deletion function. 
class DataSaver
{
private:
    // Void pointer to save pointer to any data
    void* Ptr = nullptr;
    
    // Pointer to data type saver
    DataTypeSaver* DataType = nullptr;
    
    // Variable to store data type size
    std::size_t DataTypeSize = 0;
    
    // Pointer to copy function. Required to DataSaver copy
    void (*CopyFunc)(void*& dst, const void* src) = nullptr;

    // Pointer to copy function. Required to delete data if it is pointer
    void (*DeleteFunc)(const void* ptr) = nullptr;

public:
    // Default constructor
    DataSaver() {}

    // Copy constructor
    DataSaver(const DataSaver& dataSaver)
    {
        // Call operator= method
        *this = dataSaver;
    }

    // Operator= definition to copy data from DataSaver.
    DataSaver& operator=(const DataSaver& dataSaver)
    {
        // We check that there is no self-bonding
        if (&dataSaver != this)
        {
            // Clear Ptr if it was data before
            if (Ptr != nullptr)
                free(Ptr);

            // Clear DataType if it was data before
            if (DataType != nullptr)
                delete DataType;

            // Check that dataSaver is not empty
            if (dataSaver.Ptr != nullptr)
            {
                // Create new DataType object copying data type from dataSaver
                DataType = new DataTypeSaver(dataSaver.DataType->GetDataType());
                // Copying data type size from dataSaver
                DataTypeSize = dataSaver.DataTypeSize;
                // Set new CopyFunc from dataSaver
                CopyFunc = dataSaver.CopyFunc;
                // Call new copy func to copy data from dataSaver void pointer to local void pointer. Data to Ptr will be allocated inside function
                CopyFunc(Ptr, dataSaver.Ptr);
                // Set delete function from dataSaver
                DeleteFunc = dataSaver.DeleteFunc;
            }
            else
            {
                // If dataSaver is empty clear the variables
                Ptr = nullptr;
                DataType = nullptr;
            }
        }

        return *this;
    }

    // Template constructor to save data inside DataSaver
    template<class T>
    DataSaver(const T& data)
    {
        SetData(data);
    }

    // Template constructor to save data and delete function inside DataSaver
    template<class T, class F>
    DataSaver(const T& data, F&& deleteFunc)
    {
        SetData(data, deleteFunc);
    }

    // Template method to save data inside DataSaver
    template <class T>
    void SetData(const T& data)
    {
        SetData(data, nullptr);
    }

    // Template method to save data and delete function inside DataSaver
    template <class T, class F>
    void SetData(const T& data, F&& deleteFunc)
    {
        // Clear Ptr if it was data before
        if (Ptr != nullptr)
            free(Ptr);

        // Clear DataType if it was data before
        if (DataType != nullptr)
            delete DataType;

        // Create new T type object and save it pointer like void ptr. Data from data will be copying using copy constructor
        Ptr = (void*)new T(data);

        // Create new DataType object to save data type
        DataType = new DataTypeSaver(typeid(data));
        // Save T size
        DataTypeSize = sizeof(T);

        // Set new CopyFunc. It is get to void pointers and convert void pointers to T pointers and copy data.
        CopyFunc = [](void*& dst, const void* src)
        {
            // Convert src pointer to T pointer and get data from T pointer.
            // Use T copy constructor to create T object.
            // Allocate new memory to T type and convert it to void pointer.
            dst = (void*) new T(*(T*)src);
        };

        // Set delete function from dataSaver
        DeleteFunc = deleteFunc;
    }

    // Template method to get data from DataSaver. Return true if it was data inside.
    template <class T>
    bool GetData(T& data)
    {
        // Check data type stored in DataSaver
        if (DataType != nullptr && DataType->GetDataType() != typeid(data))
        {
            std::cout << "Wrong type! Was: " + std::string(DataType->GetDataType().name()) + " Requested: " + typeid(data).name() << std::endl;
            return false;
        }

        // Check that was data inside Ptr
        if (Ptr == nullptr)
            return false;

        // Copy data from Ptr to data
        data = *(T*)Ptr;
        return true;
    }

    // Call DeleteFunc to custom data deletion. If the data deletion function has not been installed, then this method will not do anything
    void DeleteData()
    {
        if (DeleteFunc != nullptr)
        {
            DeleteFunc(Ptr);
            DeleteFunc = nullptr;
        }
    }

    // Destructor
    ~DataSaver()
    {
        if (Ptr != nullptr)
            free(Ptr);

        if (DataType != nullptr)
            delete DataType;
    }
};

// A container class for storing data of any type. Allows to specify required container.
// Container have to support find(), begin(), end(), emplace(), erase()
// If a pointer is written to one of the elements, then you can set a custom data deletion function. 
template <class C>
class DataContainer
{
protected:
    // Hash map to store all DataSaver's
    C Data;
    
public:
    // Iterators
    typedef typename C::iterator iterator;
    typedef typename C::const_iterator const_iterator;

    // Begin and end functions to work with iterators and foreach loop
    inline iterator begin() noexcept { return Data.begin(); }
    inline const_iterator cbegin() const noexcept { return Data.cbegin(); }
    inline iterator end() noexcept { return Data.end(); }
    inline const_iterator cend() const noexcept { return Data.cend(); }

    // Method to add new element with data inside container
    template <class T>
    void AddData(const std::string& key, const T& data)
    {
        Data.emplace(key, DataSaver(data));
    }

    // Method to add new element with data and custom delete function inside container
    template <class T, class F>
    void AddData(const std::string& key, const T& data, F&& deleteFunc)
    {
        Data.emplace(key, DataSaver(data, deleteFunc));
    }

    // Method to update element data
    template <class T>
    void SetData(const std::string& key, const T& data)
    {
        SetData(key, data, nullptr);
    }

    // Method to update element data and delete function
    template <class T, class F>
    void SetData(const std::string& key, const T& data, F&& deleteFunc)
    {
        auto f = Data.find(key);

        if (f == Data.end())
            AddData(key, data, deleteFunc);
        else
            f->second.SetData(data, deleteFunc);
    }

    // Method to get data from container using key
    template <class T>
    bool GetData(const std::string& key, T& data)
    {
        auto f = Data.find(key);
        if (f == Data.end())
            return false;

        f->second.GetData(data);
        return true;
    }

    // Method for checking the presence of data in the container
    bool IsData(const std::string& key)
    {
        return Data.find(key) != Data.end();
    }

    // Custom data deletion
    void DeleteData(const std::string& key)
    {
        auto f = Data.find(key);
        if (f != Data.end())
        {
            f->second.DeleteData();
            Data.erase(key);
        }
    }
};

// A container class for storing data of any type. 
// As the base class of the container, std::unordered_map is used, which is a hash map
// If a pointer is written to one of the elements, then you can set a custom data deletion function. 
class DataHashMap : public DataContainer<std::unordered_map<std::string, DataSaver>> {};

// A container class for storing data of any type. 
// As the base class of the container, std::unordered_multimap is used, which is a hash map
// If a pointer is written to one of the elements, then you can set a custom data deletion function. 
class DataMultiHashMap : public DataContainer<std::unordered_multimap<std::string, DataSaver>>
{
public:
    // Returns a pair of iterators. The first iterator points to the first element with the same key, and the second to the last element
    // Equals std::unordered_multimap<...>::equal_range
    std::pair<DataContainer::iterator, DataContainer::iterator> GetAllData(const std::string& key)
    {
        return Data.equal_range(key);
    }
};

// A class that provides access to data inside DataStorage.
// This class works as a reference to the data inside the DataStorage, 
// i.e. when the data inside this class changes, they will also change inside the DataStorage
class DataStorageRecord
{
private:
    // DataStorage record pointer
    DataHashMap* Data = nullptr;
    // Data storage param map
    DataHashMap* KeysMaps = nullptr;
public:
    DataStorageRecord() {}
    DataStorageRecord(DataHashMap* data, DataHashMap* paramMaps) : Data(data), KeysMaps(paramMaps) {}

    void SetDataHashMapPtr(DataHashMap* data) { Data = data; }
    void SetParamMapsPtr(DataHashMap* paramMaps) { KeysMaps = paramMaps; }

    // Method to update data inside DataStorage
    template <class T>
    void SetData(const std::string& key, const T& data)
    {
        // A pointer for storing a std::unordered_map in which a template data type is used as a key, 
        // and a pointer to the DataHashMap is used as a value
        std::unordered_map<T, DataHashMap*>* pointer = nullptr;
        KeysMaps->GetData(key, pointer);

        // Get the current value of the key key inside the DataStorageRecord and save it for further work
        T oldData;
        Data->GetData(key, oldData);

        // Finding data by the oldData key and save it to iterator
        auto KeyToRecordIt = pointer->find(oldData);
        // Delete data from DataStorage using key
        KeyToRecordIt->second->DeleteData(key);
        // Add new data to DataStorage
        KeyToRecordIt->second->AddData(key, data);
        
        // Remove oldData from pointer to DataStorage KeysMaps
        pointer->erase(oldData);
        // Add new data to pointer to DataStorage KeysMaps
        pointer->emplace(data, Data);
        
        // Update local DataStorageRecord data
        Data->SetData(key, data);
    }

    // Method to get data from container using key
    template <class T>
    bool GetData(const std::string& key, T& data)
    {
        return Data->GetData(key, data);
    }
};

// A class for storing a set of data of any type using a set of keys of any type
class DataStorage
{
private:
    // DataHashMap that stores a template for each new records inside DataStorage
    DataHashMap RecordTemplate;
    // 
    DataHashMap KeysMaps;
    std::unordered_map<std::string, std::function<void(DataHashMap*)>> DataStorageRecordFillers;
    std::list<DataHashMap*> RecordsList;
public:

    template <class T>
    void AddParam(const std::string& paramName, T defaultParamValue)
    {
        RecordTemplate.AddData(paramName, defaultParamValue);

        std::unordered_map<T, DataHashMap*>* pointer = new std::unordered_map<T, DataHashMap*>;
        KeysMaps.AddData(paramName, pointer, [](const void* ptr)
            {
                delete *(std::unordered_map<T, DataHashMap*>**)ptr;
            }
        );

        DataStorageRecordFillers.emplace(paramName, [=](DataHashMap* newRecord)
            {
                pointer->emplace(defaultParamValue, newRecord);
            }
        );
    }

    DataStorageRecord CreateNewRecord()
    {
        DataHashMap* newData = new DataHashMap(RecordTemplate);
        RecordsList.emplace_back(newData);

        for (auto& it : DataStorageRecordFillers)
            it.second(newData);

        return DataStorageRecord(newData, &KeysMaps);
    }

    template <class T>
    bool GetRecord(const std::string& paramName, const T& paramValue, DataStorageRecord& foundedRecord)
    {
        std::unordered_map<T, DataHashMap*>* reqMap = nullptr;
        
        if (KeysMaps.GetData(paramName, reqMap))
        {
            auto f = reqMap->find(paramValue);
            if (f != reqMap->end())
            {
                foundedRecord.SetDataHashMapPtr(f->second);
                foundedRecord.SetParamMapsPtr(&KeysMaps);
                return true;
            }
            else
                return false;
        }
        else
            return false;
    }

    ~DataStorage()
    {
        for (auto& it : KeysMaps)
            it.second.DeleteData();
        
        for (auto& it : RecordsList)
            delete it;
    }
};

int main()
{
    // Known issues: data saver cannot store arrays
    DataStorage ds;
    ds.AddParam("id", -1);
    ds.AddParam<std::string>("name", "");

    DataStorageRecord dse = ds.CreateNewRecord();

    if (ds.GetRecord<int>("id", -1, dse))
    {
        std::string res;
        if(dse.GetData("name", res))
            std::cout << "1: " << res << std::endl;
    }

    dse.SetData("id", 0);
    dse.SetData<std::string>("name", "mrognor");

    if (ds.GetRecord<int>("id", -1, dse))
    {
        std::string res;
        if(dse.GetData("name", res))
            std::cout << "2: " << res << std::endl;
    }

    if (ds.GetRecord<int>("id", 0, dse))
    {
        std::string res;
        if(dse.GetData("name", res))
            std::cout << "3: " << res << std::endl;
    }

    if (ds.GetRecord<std::string>("name", "mrognor", dse))
    {
        int res;
        if (dse.GetData("id", res))
            std::cout << "4: " << res << std::endl;
    }

    dse = ds.CreateNewRecord();
    dse.SetData("id", 1);
    dse.SetData<std::string>("name", "moop");

    if (ds.GetRecord<int>("id", 1, dse))
    {
        std::string res;
        if(dse.GetData("name", res))
            std::cout << "5: " << res << std::endl;
    }

    if (ds.GetRecord<std::string>("name", "moop", dse))
    {
        int res;
        if (dse.GetData("id", res))
            std::cout << "6: " << res << std::endl;
    }

    dse.SetData("id", 2);
    if (ds.GetRecord<std::string>("name", "moop", dse))
    {
        int res;
        if (dse.GetData("id", res))
            std::cout << "7: " << res << std::endl;
    }
}