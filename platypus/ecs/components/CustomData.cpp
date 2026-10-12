#include "CustomData.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    size_t get_custom_data_type_size(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::NONE: return 0;
            case CustomDataType::INT: return sizeof(int32_t);
            case CustomDataType::UINT: return sizeof(uint32_t);
            case CustomDataType::FLOAT: return sizeof(float);
            case CustomDataType::STRING: return sizeof(int32_t);
            case CustomDataType::VECTOR2F: return sizeof(Vector2f);
            case CustomDataType::VECTOR3F: return sizeof(Vector3f);
            case CustomDataType::VECTOR4F: return sizeof(Vector4f);
        }
        return 0;
    }

    std::string custom_data_type_to_string(CustomDataType type)
    {
        switch (type)
        {
            case CustomDataType::NONE: return "NONE";
            case CustomDataType::INT: return "INT";
            case CustomDataType::UINT: return "UINT";
            case CustomDataType::FLOAT: return "FLOAT";
            case CustomDataType::STRING: return "STRING";
            case CustomDataType::VECTOR2F: return "VECTOR2F";
            case CustomDataType::VECTOR3F: return "VECTOR3F";
            case CustomDataType::VECTOR4F: return "VECTOR4F";
        }
        return "Invalid type";
    }

    std::vector<CustomDataType> get_available_custom_data_types()
    {
        return {
            CustomDataType::INT,
            CustomDataType::UINT,
            CustomDataType::FLOAT,
            CustomDataType::STRING,
            CustomDataType::VECTOR2F,
            CustomDataType::VECTOR3F,
            CustomDataType::VECTOR4F
        };
    }


    CustomData* create_custom_data(
        entityID_t target,
        const std::vector<SerializedCustomDataValue>& values,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_custom_data"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_CUSTOM_DATA;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "Failed to allocate Transform component for entity: " + std::to_string(target),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        CustomData* pCustomData = reinterpret_cast<CustomData*>(pComponent);

        CustomDataManager& customDataManager = pScene->getCustomDataManager();
        int32_t offset = -1;

        // TODO: Figure out what to do if adding string values initially?
        //  -> how to handle deserialization?
        //for (const SerializedCustomDataValue& value : values)
        //{
        //    offset = customDataManager.add(
        //        pCustomData,
        //        value.type,
        //        value.data.data()
        //    );
        //}
        if (!values.empty())
        {
            PLATYPUS_UNIMPLEMENTED;
            PLATYPUS_ASSERT(offset != -1);
        }

        pCustomData->offset = offset;
        pCustomData->elementCount = static_cast<uint32_t>(values.size());

        return pCustomData;
    }

    size_t get_serialized_custom_data_value_size(const CustomDataValue * const pCustomDataValue)
    {
        return sizeof(CustomDataType) +
            sizeof(uint32_t) +
            pCustomDataValue->dataSize;
    }

    size_t get_serialized_custom_data_size(const CustomData * const pCustomData)
    {
        // NOTE: WARNING! Issue if this func is used outside of the current scene!!!
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        CustomDataManager& customDataManager = pScene->getCustomDataManager();
        std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);

        size_t serializedValuesSize = 0;
        for (const CustomDataValue& value : values)
            serializedValuesSize += get_serialized_custom_data_value_size(&value);

        return sizeof(ComponentType) +
            sizeof(uint32_t) + // value(element) count
            serializedValuesSize;
    }

    size_t get_serialized_custom_data_size(const char* pSerializedData, size_t dataSize)
    {
        PLATYPUS_ASSERT(dataSize >= serialized_custom_data_base_size);
        ComponentType componentType;
        memcpy(&componentType, pSerializedData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_CUSTOM_DATA);
        size_t offset = sizeof(ComponentType);

        uint32_t elementCount;
        memcpy(&elementCount, pSerializedData + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        size_t valueOffset = offset;
        size_t totalSize = serialized_custom_data_base_size;
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            valueOffset += sizeof(CustomDataType);
            uint32_t valueDataSize = 0;
            memcpy(&valueDataSize, pSerializedData + valueOffset, sizeof(uint32_t));
            totalSize += sizeof(CustomDataType) +
                sizeof(uint32_t) + // element count
                static_cast<size_t>(valueDataSize); // value data size
        }

        return totalSize;
    }

    /*
        Serialized format:
            CustomDataType type
            uint32_t dataSize
            uint8_t dataBuffer[dataSize]

        NOTE: IMPORTANT!
            *If value is string, the whole string is serialized (not the int32_t stringPoolOffset)!
                ->When deserializing this, it allocs the string from the pool automatically
    */
    CONTINUE HERE + READ ABOVE NOTE!!
    std::vector<char> serialize(const CustomDataValue * const pCustomDataValue)
    {
        const size_t serializedSize = get_serialized_custom_data_value_size(pCustomDataValue);
        std::vector<char> serializedData(serializedSize);
        char* pBuf = serializedData.data();
        memcpy(pBuf, &pCustomDataValue->type, sizeof(CustomDataType));
        size_t offset = sizeof(CustomDataType);

        const uint32_t serializedDataSize = pCustomDataValue->dataSize;
        memcpy(pBuf + offset, &serializedDataSize, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(pBuf + offset, pCustomDataValue->pData, serializedDataSize);
        offset += serializedDataSize;
        PLATYPUS_ASSERT(offset == serializedSize);

        return serializedData;
    }

    /*
        Serialized format:
            ComponentType type
            uint32_t elementCount
            CustomDataValue(serialized form) values[elementCount]
    */
    std::vector<char> serialize(const CustomData * const pCustomData)
    {
        // NOTE: WARNING! Issue if this func is used outside of the current scene!!!
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        CustomDataManager& customDataManager = pScene->getCustomDataManager();

        const size_t serializedSize = get_serialized_custom_data_size(pCustomData);
        std::vector<char> serializedData(serializedSize);

        char* pBuf = serializedData.data();
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_CUSTOM_DATA;
        memcpy(pBuf, &componentType, sizeof(ComponentType));
        size_t offset = sizeof(ComponentType);

        memcpy(pBuf + offset, &pCustomData->elementCount, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<CustomDataValue> values = customDataManager.getValues(pCustomData->offset);
        std::vector<char> fullSerializedValuesData;
        // NOTE: No idea does this work, did this quite tired..
        for (const CustomDataValue& value : values)
        {
            std::vector<char> serializedValueData = serialize(&value);
            memcpy(pBuf + offset, serializedValueData.data(), serializedValueData.size());
            offset += serializedValueData.size();
        }

        return serializedData;
    }

    void deserialize(
        Scene* pScene,
        CustomData** ppCustomData,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        const size_t baseSize = sizeof(ComponentType) +
            sizeof(uint32_t); // elementCount(valueCount)

        PLATYPUS_ASSERT(dataSize > baseSize);

        const uint8_t* pBuf = reinterpret_cast<const uint8_t*>(pData);

        ComponentType componentType;
        memcpy(&componentType, pBuf, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_TRANSFORM);
        size_t offset = sizeof(ComponentType);

        uint32_t elementCount;
        memcpy(&elementCount, pBuf + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<SerializedCustomDataValue> serializedValues(elementCount);
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            CustomDataType valueType;
            memcpy(&valueType, pBuf + offset, sizeof(CustomDataType));
            offset += sizeof(CustomDataType);

            uint32_t valueSize;
            memcpy(&valueSize, pBuf + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            std::vector<uint8_t> valueData(valueSize);
            memcpy(valueData.data(), pBuf + offset, valueSize);
            offset += valueSize;

            serializedValues[i] = {
                valueType,
                valueSize,
                valueData
            };
        }

        // TODO:
        // How the fuck assemble the actual CustomData components in Scene's "post deserialization"
        // -> needed for constructing the actual component:
        //  *elem count
        //  *serialized value data

        // NOTE: While adding this noticed that the Scene's deserialization "special cases" needs
        // to be more streamlined and coherent!
        // TODO: Make "special cases" of Scene's deserialization more streamlined
        // *Additional note: don't make this an additional "hard coded deserialization case"
        //CONTINUE HERE + READ THE FUCKING COMMENTS ABOVE!

        *ppCustomData = create_custom_data(
            entityID,
            serializedValues,
            pScene,
            true
        );
    }


    CustomDataManager::CustomDataManager(StringPool& stringPool) :
        _stringPoolRef(stringPool),
        _memoryPool(
            free_storage_func,
            this,
            validate_free_range_func,
            this
        )
    {
    }

    int32_t CustomDataManager::addNumber(
        CustomData* pCustomData,
        CustomDataType type,
        const void* pData
    )
    {
        if ((pCustomData->offset == -1 && pCustomData->elementCount != 0) ||
            (pCustomData->offset != -1 && pCustomData->elementCount == 0))
        {
            Debug::log(
                "If pCustomData->offset == -1 elementCount must be 0 and vice versa",
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return -1;
        }

        const size_t dataSize = get_custom_data_type_size(type);
        if (pCustomData->offset == -1 && pCustomData->elementCount == 0)
        {
            const size_t totalSize =
                sizeof(uint32_t) + // total size
                sizeof(CustomDataType) + // value type
                dataSize; // value data

            std::vector<uint8_t> toAdd(totalSize);
            const uint32_t uTotalSize = static_cast<const uint32_t>(totalSize);
            memcpy(toAdd.data(), &uTotalSize, sizeof(uint32_t));
            memcpy(toAdd.data() + sizeof(uint32_t), &type, sizeof(CustomDataType));
            memcpy(toAdd.data() + sizeof(uint32_t) + sizeof(CustomDataType), pData, dataSize);
            const int32_t newOffset = _memoryPool.add(
                -1,
                0,
                toAdd.size(),
                toAdd.data()
            );
            PLATYPUS_ASSERT(newOffset != -1);
            pCustomData->offset = newOffset;
            ++pCustomData->elementCount;
            return newOffset;
        }

        std::vector<StoredCustomDataValue> currentValues = copyValues(pCustomData->offset);
        StoredCustomDataValue newValue = { type };
        newValue.data.resize(dataSize);
        memcpy(newValue.data.data(), pData, dataSize);
        currentValues.push_back(newValue);

        const uint32_t newElemCount = pCustomData->elementCount + 1;
        PLATYPUS_ASSERT(newElemCount == currentValues.size());

        // DynamicElementSizeMemoryPool should handle this well enough?
        // NOT: NOT TESTED!
        // TODO: Make sure the mem is used efficiently enought with this in all cases!
        const uint8_t* pStorage = _memoryPool.getStorage().data();
        const size_t currentOffset = static_cast<const size_t>(pCustomData->offset);
        uint32_t currentSize = 0;
        memcpy(&currentSize, pStorage + currentOffset, sizeof(uint32_t));
        const uint32_t newSize = currentSize + sizeof(CustomDataType) + dataSize;

        _memoryPool.freeRange(pCustomData->offset, currentSize);

        std::vector<uint8_t> toAdd(static_cast<size_t>(newSize));
        memcpy(toAdd.data(), &newSize, sizeof(uint32_t));
        size_t toAddOffset = sizeof(uint32_t);
        for (size_t i = 0; i < newElemCount; ++i)
        {
            const StoredCustomDataValue valueToAdd = currentValues[i];
            const size_t requiredValueSize = get_custom_data_type_size(valueToAdd.type);
            const size_t valueSize = valueToAdd.data.size();
            PLATYPUS_ASSERT(requiredValueSize == valueSize);

            memcpy(
                toAdd.data() + toAddOffset,
                &valueToAdd.type,
                sizeof(CustomDataType)
            );
            toAddOffset += sizeof(CustomDataType);

            memcpy(
                toAdd.data() + toAddOffset,
                valueToAdd.data.data(),
                valueSize
            );
            toAddOffset += valueSize;
        }

        int32_t newOffset = _memoryPool.add(
            -1,
            0,
            toAdd.size(),
            toAdd.data()
        );
        PLATYPUS_ASSERT(newOffset != -1);
        pCustomData->elementCount = newElemCount;
        pCustomData->offset = newOffset;
        return newOffset;
    }

    // NOTE: Just testing atm!
    int32_t CustomDataManager::addString(
        CustomData* pCustomData,
        const std::string& str
    )
    {
        int32_t stringPoolOffset = _stringPoolRef.add(str);
        PLATYPUS_ASSERT(stringPoolOffset != -1);
        return addNumber(
            pCustomData,
            CustomDataType::STRING,
            &stringPoolOffset
        );
    }

    void CustomDataManager::updateNumber(
        int32_t offset,
        size_t valueIndex,
        CustomDataType valueType,
        const void* pValueData
    )
    {
        PLATYPUS_ASSERT(offset != -1);
        const size_t baseOffset = static_cast<const size_t>(offset);
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        uint8_t* pStorage = _memoryPool.accessStorage().data();
        PLATYPUS_ASSERT(baseOffset < totalStorageSize);

        uint32_t storedSize = 0;
        memcpy(&storedSize, pStorage + baseOffset, sizeof(uint32_t));
        size_t dataOffset = baseOffset + sizeof(uint32_t);
        PLATYPUS_ASSERT(dataOffset < totalStorageSize);

        const uint32_t valueTypeSize = static_cast<uint32_t>(get_custom_data_type_size(valueType));
        size_t currentValueIndex = 0;
        while (dataOffset < baseOffset + storedSize)
        {
            CustomDataType currentValueType = CustomDataType::NONE;
            memcpy(&currentValueType, pStorage + dataOffset, sizeof(CustomDataType));
            dataOffset += sizeof(CustomDataType);
            size_t currentValueSize = get_custom_data_type_size(currentValueType);
            if (valueIndex == currentValueIndex)
            {
                PLATYPUS_ASSERT(valueType == currentValueType);
                PLATYPUS_ASSERT(valueTypeSize == currentValueSize);
                memcpy(pStorage + dataOffset, pValueData, valueTypeSize);
                return;
            }

            dataOffset += currentValueSize;
            ++currentValueIndex;
        }

        Debug::log(
            "Value index " + std::to_string(valueIndex) + " out of bounds. "
            "Offset " + std::to_string(baseOffset) + " has " + std::to_string(currentValueIndex) + " values.",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }

    void CustomDataManager::updateString(
        int32_t offset,
        size_t valueIndex,
        const std::string& str
    )
    {
        PLATYPUS_ASSERT(offset <= _memoryPool.getTotalSize());
        const int32_t stringPoolOffset = getNumericValue<int32_t>(offset, valueIndex);
        PLATYPUS_ASSERT(stringPoolOffset != -1);
        const int32_t newStringPoolOffset = _stringPoolRef.update(stringPoolOffset, str);
        updateNumber(offset, valueIndex, CustomDataType::STRING, &newStringPoolOffset);
    }

    void CustomDataManager::remove(CustomData* pCustomData, size_t valueIndex)
    {
        PLATYPUS_ASSERT(pCustomData->offset != -1);
        const size_t baseOffset = static_cast<const size_t>(pCustomData->offset);
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        uint8_t* pStorage = _memoryPool.accessStorage().data();
        PLATYPUS_ASSERT(baseOffset < totalStorageSize);

        uint32_t storedSize = 0;
        memcpy(&storedSize, pStorage + baseOffset, sizeof(uint32_t));
        size_t dataOffset = baseOffset + sizeof(uint32_t);
        PLATYPUS_ASSERT(dataOffset < totalStorageSize);

        size_t currentValueIndex = 0;
        while (dataOffset < baseOffset + storedSize)
        {
            CustomDataType currentValueType = CustomDataType::NONE;
            memcpy(&currentValueType, pStorage + dataOffset, sizeof(CustomDataType));
            const size_t dataBaseOffset = dataOffset;
            dataOffset += sizeof(CustomDataType);
            size_t currentValueDataSize = get_custom_data_type_size(currentValueType);
            if (valueIndex == currentValueIndex)
            {
                if (currentValueType == CustomDataType::STRING)
                {
                    int32_t stringPoolOffset = -1;
                    memcpy(&stringPoolOffset, pStorage + dataOffset, sizeof(int32_t));
                    _stringPoolRef.remove(stringPoolOffset);
                }

                // Pack the remaining values so those are contiguous + change the values size
                const size_t currentValueTotalSize = sizeof(CustomDataType) + currentValueDataSize;
                const uint32_t newTotalSize = storedSize - currentValueTotalSize;
                const size_t remainingSize = newTotalSize - (dataBaseOffset - baseOffset);
                const size_t nextValueOffset = dataBaseOffset + currentValueTotalSize;
                PLATYPUS_ASSERT(nextValueOffset <= totalStorageSize);

                // Pack the remaining values to be contiguous
                std::vector<uint8_t> remainingData(remainingSize);
                memcpy(remainingData.data(), pStorage + nextValueOffset, remainingSize);
                memcpy(pStorage + dataBaseOffset, remainingData.data(), remainingSize);

                // Set the last "emptied value" to 0
                const size_t lastOffset = baseOffset + storedSize - currentValueTotalSize;
                memset(pStorage + lastOffset, 0, currentValueTotalSize);

                _memoryPool.accessFreeRanges()[lastOffset] = currentValueTotalSize;
                _memoryPool.packFreeRanges();

                // Update the changed total size and count
                memcpy(pStorage + baseOffset, &newTotalSize, sizeof(uint32_t));
                --pCustomData->elementCount;

                return;
            }

            dataOffset += currentValueDataSize;
            ++currentValueIndex;
        }

        Debug::log(
            "Value index " + std::to_string(valueIndex) + " out of bounds. "
            "Offset " + std::to_string(baseOffset) + " has " + std::to_string(currentValueIndex) + " values.",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }

    std::vector<CustomDataValue> CustomDataManager::getValues(int32_t offset) const
    {
        const size_t storageSize = _memoryPool.getTotalSize();
        PLATYPUS_ASSERT(offset + sizeof(uint32_t) <= storageSize);
        const std::vector<uint8_t>& storage = _memoryPool.getStorage();
        const uint8_t* pStorage = storage.data();

        uint32_t size = 0;
        memcpy(&size, pStorage + offset, sizeof(uint32_t));
        PLATYPUS_ASSERT(size >= sizeof(uint32_t));

        std::vector<CustomDataValue> values;
        size_t dataOffset = static_cast<size_t>(offset) + sizeof(uint32_t);
        while (dataOffset < offset + size)
        {
            CustomDataType type = CustomDataType::NONE;
            memcpy(&type, pStorage + dataOffset, sizeof(CustomDataType));
            PLATYPUS_ASSERT(type != CustomDataType::NONE);
            dataOffset += sizeof(CustomDataType);
            PLATYPUS_ASSERT(dataOffset < storageSize);

            const size_t storedDataSize = get_custom_data_type_size(type);
            values.push_back({
                type,
                static_cast<uint32_t>(storedDataSize),
                pStorage + dataOffset
            });
            dataOffset += storedDataSize;
        }

        return values;
    }

    template<typename T>
    T CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const
    {
        T value;
        const void* pValueData = getValueData(
            offset,
            sizeof(T),
            valueIndex
        );
        memcpy(&value, pValueData, sizeof(T));
        return value;
    }

    template int32_t    CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;
    template uint32_t   CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;
    template float      CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;
    template Vector2f   CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;
    template Vector3f   CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;
    template Vector4f   CustomDataManager::getNumericValue(int32_t offset, size_t valueIndex) const;


    std::string CustomDataManager::valueToString(const CustomDataValue& value) const
    {
        if (value.type == CustomDataType::INT)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(int32_t));
            return std::to_string(*reinterpret_cast<const int32_t*>(value.pData));
        }
        else if (value.type == CustomDataType::UINT)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(uint32_t));
            return std::to_string(*reinterpret_cast<const uint32_t*>(value.pData));
        }
        else if (value.type == CustomDataType::FLOAT)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(float));
            float fVal = 0.0f;
            memcpy(&fVal, value.pData, sizeof(float));
            return std::to_string(fVal);
        }
        else if (value.type == CustomDataType::VECTOR2F)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(Vector2f));
            Vector2f vec;
            memcpy(&vec, value.pData, sizeof(Vector2f));
            return vec.toString();
        }
        else if (value.type == CustomDataType::VECTOR3F)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(Vector3f));
            Vector3f vec;
            memcpy(&vec, value.pData, sizeof(Vector3f));
            return vec.toString();
        }
        else if (value.type == CustomDataType::VECTOR4F)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(Vector4f));
            Vector4f vec;
            memcpy(&vec, value.pData, sizeof(Vector4f));
            return vec.toString();
        }
        else if (value.type == CustomDataType::STRING)
        {
            PLATYPUS_ASSERT(value.dataSize == sizeof(int32_t));
            int32_t stringPoolOffset = -1;
            memcpy(&stringPoolOffset, value.pData, sizeof(int32_t));
            return _stringPoolRef.get(stringPoolOffset);
        }

        Debug::log(
            "Invalid type: " + custom_data_type_to_string(value.type),
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return "";
    }

    std::string CustomDataManager::getStringValue(const CustomData * const pCustomData, size_t valueIndex) const
    {
        const int32_t stringPoolOffset = getNumericValue<int32_t>(pCustomData->offset, valueIndex);
        return _stringPoolRef.get(stringPoolOffset);
    }

    const void* CustomDataManager::getValueData(
        size_t baseOffset,
        size_t valueSize,
        size_t valueIndex
    ) const
    {
        const size_t storageSize = _memoryPool.getTotalSize();
        PLATYPUS_ASSERT(baseOffset + sizeof(uint32_t) <= storageSize);
        const std::vector<uint8_t>& storage = _memoryPool.getStorage();
        const uint8_t* pStorage = storage.data();

        uint32_t size = 0;
        memcpy(&size, pStorage + baseOffset, sizeof(uint32_t));
        PLATYPUS_ASSERT(size >= sizeof(uint32_t));

        size_t dataOffset = static_cast<size_t>(baseOffset) + sizeof(uint32_t);
        size_t currentIndex = 0;
        while (dataOffset < baseOffset + size)
        {
            CustomDataType storedType = CustomDataType::NONE;
            memcpy(&storedType, pStorage + dataOffset, sizeof(CustomDataType));
            PLATYPUS_ASSERT(storedType != CustomDataType::NONE);
            dataOffset += sizeof(CustomDataType);
            PLATYPUS_ASSERT(dataOffset < storageSize);
            const size_t storedDataSize = get_custom_data_type_size(storedType);

            if (currentIndex == valueIndex)
            {
                PLATYPUS_ASSERT(storedDataSize == valueSize);
                return pStorage + dataOffset;
            }

            dataOffset += storedDataSize;
            ++currentIndex;
        }

        //  Never supposed to get here!
        Debug::log(
            "Value index " + std::to_string(valueIndex) + " out of bounds. "
            "Offset " + std::to_string(baseOffset) + " has " + std::to_string(currentIndex) + " values.",
            PLATYPUS_CURRENT_FUNC_NAME,
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return nullptr;
    }

    std::vector<CustomDataManager::StoredCustomDataValue> CustomDataManager::copyValues(
        int32_t offset
    ) const
    {
        const size_t totalStorageSize = _memoryPool.getTotalSize();
        if (!validateOffset(offset))
        {
            Debug::log(
                "Invalid offset " + std::to_string(offset) + " "
                "MemoryPool size is " + std::to_string(totalStorageSize),
                PLATYPUS_CURRENT_FUNC_NAME,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return { };
        }

        const uint8_t* pStorage = _memoryPool.getStorage().data();

        uint32_t storedSize = 0;
        memcpy(&storedSize, pStorage + offset, sizeof(uint32_t));
        PLATYPUS_ASSERT(offset + storedSize <= totalStorageSize);
        size_t dataOffset = offset + sizeof(uint32_t);

        std::vector<StoredCustomDataValue> result;
        while (dataOffset < offset + storedSize)
        {
            CustomDataType valueType = CustomDataType::NONE;
            memcpy(&valueType, pStorage + dataOffset, sizeof(CustomDataType));
            dataOffset += sizeof(CustomDataType);

            const size_t valueSize = get_custom_data_type_size(valueType);
            StoredCustomDataValue valueCpy = { valueType };
            valueCpy.data.resize(valueSize);
            memcpy(valueCpy.data.data(), pStorage + dataOffset, valueSize);
            result.push_back(valueCpy);

            dataOffset += valueSize;
        }
        return result;
    }

    bool CustomDataManager::validateOffset(int32_t offset) const
    {
        return (offset != -1) && (offset < _memoryPool.getTotalSize());
    }

    void CustomDataManager::free_storage_func(size_t offset, size_t size, void* pUserData)
    {
        CustomDataManager* pCustomDataManager = reinterpret_cast<CustomDataManager*>(pUserData);
        DynamicElementSizeMemoryPool& memoryPool = pCustomDataManager->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        memset(memoryPool.accessStorage().data() + offset, 0, size);
    }

    bool CustomDataManager::validate_free_range_func(size_t offset, size_t size, void* pUserData)
    {
        const CustomDataManager* pCustomDataManager = reinterpret_cast<const CustomDataManager*>(pUserData);
        const DynamicElementSizeMemoryPool& memoryPool = pCustomDataManager->_memoryPool;
        PLATYPUS_ASSERT(offset + size <= memoryPool.getTotalSize());
        std::vector<uint8_t> emptyBytes(size);
        memset(emptyBytes.data(), 0, size);
        return memcmp(memoryPool.getStorage().data() + offset, emptyBytes.data(), size) == 0;
    }
}
