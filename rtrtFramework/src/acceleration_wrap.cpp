
#include "acceleration_wrap.h"
#include "vkapp.h"
#include <numeric>
#include "buffer_wrap.h"
#undef MemoryBarrier

//--------------------------------------------------------------------------------------------------
// Initializing the allocator and querying the raytracing properties
//

void RaytracingBuilderKHR::setup(VkApp* _VK, const vk::Device& device, uint32_t queueIndex)
{
    VK = _VK;
    printf("RaytracingBuilderKHR::setup (3)\n");
    m_device     = device;
    m_queueIndex = queueIndex;
}

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void RaytracingBuilderKHR::destroy()
{
    printf("RaytracingBuilderKHR::destroy (6)\n"); 
    for(auto& blas : m_blas)  {
        blas.bw.destroy(VK->m_device);
        //vkDestroyAccelerationStructureKHR(VK->m_device, blas.accel, nullptr); 
        VK->m_device.destroyAccelerationStructureKHR(blas.accel, nullptr);
    }
    
    m_tlas.bw.destroy(VK->m_device);
    //vkDestroyAccelerationStructureKHR(VK->m_device, m_tlas.accel, nullptr);
    VK->m_device.destroyAccelerationStructureKHR(m_tlas.accel, nullptr);
    m_blas.clear();
}

//--------------------------------------------------------------------------------------------------
// Returning the constructed top-level acceleration structure
//
vk::AccelerationStructureKHR RaytracingBuilderKHR::getAccelerationStructure() const
{
    //printf("RaytracingBuilderKHR::getAccelerationStructure\n");
    return m_tlas.accel;
}

//--------------------------------------------------------------------------------------------------
// Return the device address of a Blas previously created.
//
vk::DeviceAddress RaytracingBuilderKHR::getBlasDeviceAddress(uint32_t blasId)
{
    //printf("RaytracingBuilderKHR::getBlasDeviceAddress (4)\n");
    assert(size_t(blasId) < m_blas.size());
    //VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    vk::AccelerationStructureDeviceAddressInfoKHR addressInfo;
    //addressInfo.accelerationStructure = m_blas[blasId].accel;
    addressInfo.setAccelerationStructure(m_blas[blasId].accel);
    //return vkGetAccelerationStructureDeviceAddressKHR(m_device, &addressInfo);
    return m_device.getAccelerationStructureAddressKHR(&addressInfo);
}

//--------------------------------------------------------------------------------------------------
// Create all the BLAS from the vector of BlasInput
// - There will be one BLAS per input-vector entry
// - There will be as many BLAS as input.size()
// - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
//   and can be referenced by index.
// - if flag has the 'Compact' flag, the BLAS will be compacted
//
void RaytracingBuilderKHR::buildBlas(const std::vector<BlasInput>& input,
                                     vk::BuildAccelerationStructureFlagsKHR flags)
{
    //printf("RaytracingBuilderKHR::buildBlas (110)\n");
    auto         nbBlas = static_cast<uint32_t>(input.size());
    vk::DeviceSize asTotalSize{0};     // Memory size of all allocated BLAS
    uint32_t     nbCompactions{0};   // Nb of BLAS requesting compaction
    vk::DeviceSize maxScratchSize{0};  // Largest scratch size

    // Preparing the information for the acceleration build commands.
    std::vector<BuildAccelerationStructure> buildAs(nbBlas);
    for(uint32_t idx = 0; idx < nbBlas; idx++)
        {
            // Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
            // Other information will be filled in the createBlas (see #2)
            buildAs[idx].buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
            buildAs[idx].buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
            buildAs[idx].buildInfo.setFlags(input[idx].flags | flags);
            buildAs[idx].buildInfo.setGeometryCount(static_cast<uint32_t>(input[idx].asGeometry.size()));
            buildAs[idx].buildInfo.setPGeometries(input[idx].asGeometry.data());

            // Build range information
            buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo.data();

            // Finding sizes to create acceleration structures and scratch
            std::vector<uint32_t> maxPrimCount(input[idx].asBuildOffsetInfo.size());
            for(auto tt = 0; tt < input[idx].asBuildOffsetInfo.size(); tt++)
                maxPrimCount[tt] = input[idx].asBuildOffsetInfo[tt].primitiveCount; //# of triangles
            m_device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildAs[idx].buildInfo, 
                                                            maxPrimCount.data(), &buildAs[idx].sizeInfo);
            // Extra info
            asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
            maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
            nbCompactions += hasFlag(buildAs[idx].buildInfo.flags, vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction);
        }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    VK->m_scratch1 = VK->createBufferWrap(maxScratchSize,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    //NAME(VK->m_scratch1.buffer, VK_OBJECT_TYPE_BUFFER, "buildBlas scratch buffer");
  
    vk::BufferDeviceAddressInfo bufferInfo;
    bufferInfo.setBuffer(VK->m_scratch1.buffer);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress(&bufferInfo);

    // Allocate a query pool for storing the needed size for every BLAS compaction.
    vk::QueryPool queryPool;
    if(nbCompactions > 0)  // Is compaction requested?
        {
            assert(nbCompactions == nbBlas);  // Don't allow mix of on/off compaction
            vk::QueryPoolCreateInfo qpci;
            qpci.setQueryCount(nbBlas);
            qpci.setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
            m_device.createQueryPool(&qpci, nullptr, &queryPool);
        }

    // Batching creation/compaction of BLAS to allow staying in restricted amount of memory
    std::vector<uint32_t> indices;  // Indices of the BLAS to create
    vk::DeviceSize          batchSize{0};
    vk::DeviceSize          batchLimit{256'000'000};  // 256 MB
    for(uint32_t idx = 0; idx < nbBlas; idx++)
        {
            indices.push_back(idx);
            batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
            // Over the limit or last BLAS element
            if(batchSize >= batchLimit || idx == nbBlas - 1)
                {
                    vk::CommandBuffer cmdBuf = VK->createTempCppCmdBuffer();
                    cmdCreateBlas(cmdBuf, indices, buildAs, scratchAddress, queryPool);
                    VK->submitTempCmdBuffer(cmdBuf);

                    if (queryPool)
                        {
                            vk::CommandBuffer cmdBuf = VK->createTempCppCmdBuffer();
                            cmdCompactBlas(cmdBuf, indices, buildAs, queryPool);
                            VK->submitTempCmdBuffer(cmdBuf);

                            // Destroy the non-compacted version
                            destroyNonCompacted(indices, buildAs);
                        }
                    // Reset

                    batchSize = 0;
                    indices.clear();
                }
        }

    // Logging reduction
    if(queryPool)
        {
            vk::DeviceSize compactSize = std::accumulate(buildAs.begin(), buildAs.end(), 0ULL, [](const auto& a, const auto& b) {
                return a + b.sizeInfo.accelerationStructureSize;
            });
        }

    // Keeping all the created acceleration structures
    for(auto& b : buildAs)
        {
            m_blas.emplace_back(b.as);
        }

    // Clean up
    m_device.destroyQueryPool(queryPool, nullptr);
    //scratch.destroy(m_device);
}

WrapAccelerationStructure createAcceleration(VkApp* VK,
                                              vk::AccelerationStructureCreateInfoKHR& accel_)
{
    //printf("createAcceleration (6)\n");
    WrapAccelerationStructure result;
    // Allocating the buffer to hold the acceleration structure
    result.bw = VK->createBufferWrap(accel_.size, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Create the acceleration structure
    accel_.buffer = result.bw.buffer;
    VK->m_device.createAccelerationStructureKHR(&accel_, nullptr, &result.accel);
    return result;
}

// Creating the bottom level acceleration structure for all indices of `buildAs` vector.
// The array of BuildAccelerationStructure was created in buildBlas and the vector of
// indices limits the number of BLAS to create at once. This limits the amount of
// memory needed when compacting the BLAS.
void RaytracingBuilderKHR::cmdCreateBlas(vk::CommandBuffer                          cmdBuf,
                                         std::vector<uint32_t>                    indices,
                                         std::vector<BuildAccelerationStructure>& buildAs,
                                         vk::DeviceAddress                          scratchAddress,
                                         vk::QueryPool                              queryPool)
{
    //printf("RaytracingBuilderKHR::cmdCreateBlas (40)\n");
    if(queryPool)  // For querying the compaction size
        vkResetQueryPool(m_device, queryPool, 0, static_cast<uint32_t>(indices.size()));
    uint32_t queryCnt{0};

    for(const auto& idx : indices)
        {
            // Actual allocation of buffer and acceleration structure.
            vk::AccelerationStructureCreateInfoKHR createInfo;
            createInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
            // Will be used to allocate memory.
            createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
            buildAs[idx].as = createAcceleration(VK, createInfo);
    
            // BuildInfo #2 part
            // Setting where the build lands
            buildAs[idx].buildInfo.dstAccelerationStructure  = buildAs[idx].as.accel;
            // All build use the same scratch buffer
            buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;

            // Building the bottom-level-acceleration-structure
            cmdBuf.buildAccelerationStructuresKHR(1, &buildAs[idx].buildInfo,
                                                  &buildAs[idx].rangeInfo);

            // Since the scratch buffer is reused across builds, we
            // need a barrier to ensure one build is finished before
            // starting the next one.
            //VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            vk::MemoryBarrier barrier;
            barrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR);
            //barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            barrier.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);
            //barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            //vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            //                     VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            //                     0, 1, &barrier, 0, nullptr, 0, nullptr);
            cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::DependencyFlags(), 1, &barrier, 0, nullptr, 0, nullptr);


            if(queryPool)
                {
                    // Add a query to find the 'real' amount of memory needed, use for compaction
                    //vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1,
                    //          &buildAs[idx].buildInfo.dstAccelerationStructure,
                    //           VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                    //           queryPool, queryCnt++);
                    cmdBuf.writeAccelerationStructuresPropertiesKHR(1, &buildAs[idx].buildInfo.dstAccelerationStructure,
                        vk::QueryType::eAccelerationStructureCompactedSizeKHR, queryPool, queryCnt++);
                }
        }
}

//--------------------------------------------------------------------------------------------------
// Create and replace a new acceleration structure and buffer based on the size retrieved by the
// Query.
void RaytracingBuilderKHR::cmdCompactBlas(vk::CommandBuffer                          cmdBuf,
                                          std::vector<uint32_t>                    indices,
                                          std::vector<BuildAccelerationStructure>& buildAs,
                                          vk::QueryPool                              queryPool)
{
    //printf("RaytracingBuilderKHR::cmdCompactBlas\n");
    uint32_t queryCtn{0};

    // Get the compacted size result back
    std::vector<vk::DeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
    //vkGetQueryPoolResults(m_device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
    //                      compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);
    m_device.getQueryPoolResults(queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(vk::DeviceSize),
        compactSizes.data(), sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait);

    for(auto idx : indices)
        {
            buildAs[idx].cleanupAS                          = buildAs[idx].as.accel;           // previous AS to destroy
            buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++];  // new reduced size

            // Creating a compact version of the AS
            vk::AccelerationStructureCreateInfoKHR asCreateInfo;
            asCreateInfo.setSize(buildAs[idx].sizeInfo.accelerationStructureSize);
            asCreateInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
            buildAs[idx].as = createAcceleration(VK, asCreateInfo);

            // Copy the original BLAS to a compact version
            vk::CopyAccelerationStructureInfoKHR copyInfo;
            copyInfo.setSrc(buildAs[idx].buildInfo.dstAccelerationStructure);
            copyInfo.setDst(buildAs[idx].as.accel);
            copyInfo.setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
            //vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
            cmdBuf.copyAccelerationStructureKHR(&copyInfo);
        }
}

//--------------------------------------------------------------------------------------------------
// Destroy all the non-compacted acceleration structures
//
void RaytracingBuilderKHR::destroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs)
{
    //printf("RaytracingBuilderKHR::destroyNonCompacted\n");
    for(auto& i : indices)
        {
            //vkDestroyAccelerationStructureKHR(VK->m_device, buildAs[i].cleanupAS, nullptr);
            VK->m_device.destroyAccelerationStructureKHR(buildAs[i].cleanupAS, nullptr);
        }
}

//--------------------------------------------------------------------------------------------------
// Low level of Tlas creation 
//
void RaytracingBuilderKHR::cmdCreateTlas(vk::CommandBuffer                      cmdBuf,
                                         uint32_t                             countInstance,
                                         vk::DeviceAddress                      instBufferAddr,
                                         vk::BuildAccelerationStructureFlagsKHR flags,
                                         bool                                 update,
                                         bool                                 motion)
{
    //printf("RaytracingBuilderKHR::cmdCreateTlas (75)\n");
    // Wraps a device pointer to the above uploaded instances.
    vk::AccelerationStructureGeometryInstancesDataKHR instancesVk;
    instancesVk.data.deviceAddress = instBufferAddr;

    // Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
    vk::AccelerationStructureGeometryKHR topASGeometry;
    topASGeometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    topASGeometry.geometry.instances = instancesVk;

    // Find sizes
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.setFlags(flags);
    buildInfo.setGeometryCount(1);
    buildInfo.setPGeometries(&topASGeometry);
    buildInfo.setMode( update ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild);
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildInfo.setSrcAccelerationStructure(VK_NULL_HANDLE);

    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo;
    //vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
    //                                       &countInstance, &sizeInfo);
    m_device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &buildInfo, &countInstance, &sizeInfo);
    // Create TLAS
    if(update == false)
        {

            vk::AccelerationStructureCreateInfoKHR createInfo;
            createInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
            createInfo.setSize(sizeInfo.accelerationStructureSize);
            m_tlas = createAcceleration(VK, createInfo);
        }

    // Allocate the scratch memory
    //VK->m_scratch2 = VK->createBufferWrap(sizeInfo.buildScratchSize,
    //                                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    //                                          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    //                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK->m_scratch2 = VK->createBufferWrap(sizeInfo.buildScratchSize, 
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    //NAME(VK->m_scratch2.buffer, VK_OBJECT_TYPE_BUFFER, "cmdCreateTlas scratch buffer");

    vk::BufferDeviceAddressInfo bufferInfo;
    bufferInfo.setBuffer(VK->m_scratch2.buffer);
    vk::DeviceAddress scratchAddress = m_device.getBufferAddress(&bufferInfo);
    //vkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);

    // Update build information
    buildInfo.srcAccelerationStructure  = update ? m_tlas.accel : VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure  = m_tlas.accel;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    // Build Offsets info: n instances
    vk::AccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{countInstance, 0, 0, 0};
    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS
    //vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildOffsetInfo);
    cmdBuf.buildAccelerationStructuresKHR(1, &buildInfo, &pBuildOffsetInfo);
    //scratch.destroy(VK->m_device);
}

//--------------------------------------------------------------------------------------------------
// Refit BLAS number blasIdx from updated buffer contents.
//
/*
void RaytracingBuilderKHR::updateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags)
{
    assert (false && "Not used; Not maintained;  Probably leaks a VkDeviceMemory");
    //printf("RaytracingBuilderKHR::updateBlas\n");
    assert(size_t(blasIdx) < m_blas.size());

    // Preparing all build information, acceleration is filled later
    VkAccelerationStructureBuildGeometryInfoKHR buildInfos{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfos.flags                    = flags;
    buildInfos.geometryCount            = (uint32_t)blas.asGeometry.size();
    buildInfos.pGeometries              = blas.asGeometry.data();
    buildInfos.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;  // UPDATE
    buildInfos.type                     = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfos.srcAccelerationStructure = m_blas[blasIdx].accel;  // UPDATE
    buildInfos.dstAccelerationStructure = m_blas[blasIdx].accel;

    // Find size to build on the device
    std::vector<uint32_t> maxPrimCount(blas.asBuildOffsetInfo.size());
    for(auto tt = 0; tt < blas.asBuildOffsetInfo.size(); tt++)
        maxPrimCount[tt] = blas.asBuildOffsetInfo[tt].primitiveCount;  // Number of primitives/triangles
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfos,
                                            maxPrimCount.data(), &sizeInfo);

    // Allocate the scratch buffer and setting the scratch info
    BufferWrap scratch = VK->createBufferWrap(sizeInfo.buildScratchSize,
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                              | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferInfo.buffer = scratch.buffer;
    buildInfos.scratchData.deviceAddress = vkGetBufferDeviceAddress(m_device, &bufferInfo);

    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
    for(size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
        pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

    VkCommandBuffer cmdBuf = VK->createTempCmdBuffer();

    // Update the instance buffer on the device side and build the TLAS
    // Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
    // and the existing BLAS being passed and updated in place
    vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfos, pBuildOffset.data());

    VK->submitTempCmdBuffer(cmdBuf);
}*/
    
void RaytracingBuilderKHR::buildTlas(
                                 const std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                                 vk::BuildAccelerationStructureFlagsKHR flags,
                                 bool update, bool motion)
{
    //printf("RaytracingBuilderKHR::buildTlas (30)\n");
    // Cannot call buildTlas twice except to update.
    //assert(m_tlas.accel == VK_NULL_HANDLE || update);
    uint32_t countInstance = static_cast<uint32_t>(instances.size());

    // Command buffer to create the TLAS
    vk::CommandBuffer    cmdBuf = VK->createTempCppCmdBuffer();

    // Create a buffer holding the actual instance data (matrices++) for use by the AS builder
    //BufferWrap instancesBuffer = VK->createStagedBufferWrap(cmdBuf, instances,
    //                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    //                                             | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    BufferWrap instancesBuffer = VK->createStagedBufferWrap(cmdBuf, instances,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
    
    vk::BufferDeviceAddressInfo bufferInfo;
    bufferInfo.setBuffer(instancesBuffer.buffer);
    vk::DeviceAddress instBufferAddr = m_device.getBufferAddress(&bufferInfo);
    //VkDeviceAddress           instBufferAddr = vkGetBufferDeviceAddress(m_device, &bufferInfo);
    
    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    vk::MemoryBarrier barrier;
    //barrier.setSrcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    //barrier.setDstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR);
    //vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                     VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    //                     0, 1, &barrier, 0, nullptr, 0, nullptr);
    cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
        vk::DependencyFlags(), 1, &barrier, 0, nullptr, 0, nullptr);

    // Creating the TLAS
    cmdCreateTlas(cmdBuf, countInstance, instBufferAddr, flags, update, motion);

    // Finalizing and destroying temporary data
    VK->submitTemptCppCmdBuffer(cmdBuf);
    
    instancesBuffer.destroy(VK->m_device);
 }


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Convert an OBJ model into the ray tracing geometry used to build the BLAS
//
BlasInput VkApp::objectToVkGeometryKHR(const ObjData& model)
{
    //printf("VkApp::objectToVkGeometryKHR (45)\n");
    // BLAS builder requires raw device addresses.
    vk::BufferDeviceAddressInfo _b1;
    _b1.setBuffer(model.vertexBuffer.buffer);
    vk::DeviceAddress vertexAddress = m_device.getBufferAddress(&_b1);
    //vk::DeviceAddress vertexAddress = vkGetBufferDeviceAddress(m_device, &_b1);
    
    vk::BufferDeviceAddressInfo _b2;
    _b2.setBuffer(model.indexBuffer.buffer);
    vk::DeviceAddress indexAddress = m_device.getBufferAddress(&_b2);
    //vk::DeviceAddress indexAddress  = vkGetBufferDeviceAddress(m_device, &_b2);

    uint32_t maxPrimitiveCount = model.nbIndices / 3;


    // Describe buffer as array of Vertex.
    vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
    triangles.setVertexFormat(vk::Format::eR32G32B32A32Sfloat);  // vec3 vertex position data.
    triangles.vertexData.deviceAddress = vertexAddress;
    triangles.setVertexStride(sizeof(Vertex));
    // Describe index data (32-bit unsigned int)
    triangles.setIndexType(vk::IndexType::eUint32);
    triangles.indexData.deviceAddress = indexAddress;
    // Indicate identity transform by setting transformData to null device pointer.
    //triangles.transformData = {};
    triangles.setMaxVertex(model.nbVertices);

    // Identify the above data as containing opaque triangles.
    vk::AccelerationStructureGeometryKHR asGeom;
    asGeom.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    asGeom.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    asGeom.geometry.triangles = triangles;

    // The entire array will be used to build the BLAS.
    vk::AccelerationStructureBuildRangeInfoKHR offset;
    offset.firstVertex     = 0;
    offset.primitiveCount  = maxPrimitiveCount;
    offset.primitiveOffset = 0;
    offset.transformOffset = 0;

    // Our blas is made from only one geometry, but could be made of many geometries
    BlasInput input;
    input.asGeometry.emplace_back(asGeom);
    input.asBuildOffsetInfo.emplace_back(offset);

    return input;
}

void VkApp::createRtAccelerationStructure()
{
    //printf("VkApp::createRtAccelerationStructure (25)\n");
    // BLAS - Storing each primitive in a geometry
    std::vector<BlasInput> allBlas;
    allBlas.reserve(m_objData.size());
    for (const auto& obj : m_objData)  {
        BlasInput blas = objectToVkGeometryKHR(obj);
        // We could add more geometry in each BLAS, but we add only one for now
        allBlas.emplace_back(blas); }

    //m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    m_rtBuilder.buildBlas(allBlas, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    // TLAS 
    std::vector<vk::AccelerationStructureInstanceKHR> tlas;
    tlas.reserve(m_objInst.size());
    for(const ObjInst& inst : m_objInst) {
        vk::AccelerationStructureInstanceKHR _i{};
        _i.transform = toTransformMatrixKHR(inst.transform);  // Position of the instance
        _i.instanceCustomIndex = inst.objIndex; 
        _i.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(inst.objIndex);
        _i.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        _i.mask  = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
        _i.instanceShaderBindingTableRecordOffset = 0; // Use the same hit group for all objects
        tlas.emplace_back(_i); }
    
    m_rtBuilder.buildTlas(tlas, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                          false, false);
}

