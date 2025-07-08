#pragma once

#include "srl_base.hpp"
#include "srl_tv.hpp"
#include "srl_cram.hpp"
#include "srl_ascii.hpp"
#include "srl_debug.hpp"
#include "srl_cd.hpp"
#include "srl_tilemap_interfaces.hpp"

namespace SRL
{
    /** @brief VDP2 control
     */
    class VDP2
    {
    public:

        /** @brief  VDP2 scroll priorities (Higher layers draw on top of lower layers)
         */
        enum class Priority : uint16_t
        {
            Layer0 = 0,
            Layer1 = 1,
            Layer2 = 2,
            Layer3 = 3,
            Layer4 = 4,
            Layer5 = 5,
            Layer6 = 6,
            Layer7 = 7,
        };
        /** @brief Options for using color offsets on Scroll Screens
         *  
         *  @details Scroll and sprite screens can apply one of 2 color offsets
         *  (A or B) to all pixels from that screen. 
         *  See VDP2::ScrollScreen::UseColorOffset() and VDP2::SetColorOffsetA/B() 
         *  for further details
         */
        enum class OffsetChannel
        {
            /* @brief Do not use any color offset
             */
            NoOffset,
            /* @brief Use color offset A
             */
            OffsetA,
            /* @brief Use color offset B
             */
            OffsetB,
        };

        /** @brief used to specify the 4 VRAM banks that are available for VDP2 allocation
         */
        enum class VramBank : uint16_t
        {
            A0 = 0,
            A1 = 1,
            B0 = 2,
            B1 = 3,
        };

        /** @brief Manages VDP2 VRAM allocation
         */
        class VRAM
        {
        private:

            /** @brief VDP2 class needs access to some of the internal variables
             */
            friend class VDP2;

            /** @brief Bottom RAM bank zones
             */
            inline static uint8_t* bankBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };

            /** @brief Top RAM bank zones
             */
            inline static uint8_t* bankTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };

            /** @brief Currently allocated bottom RAM bank zones
             */
            inline static uint8_t* currentBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };;

            /** @brief Currently allocated top RAM bank zones
             */
            inline static uint8_t* currentTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };

            /** @brief Number of cycles available for each bank
             * @note Indicates number of cycles that are free in the VRAM bank for per pixel operations
             */
            inline static int8_t bankCycles[4] = { -1,-1,-1,2 };//why will init to 0 not work?

        public:
            /** @brief Gets current amount of free VRAM in a bank
            * @param bank the VRAM bank to get free space in
            * @return number of available bytes in bank
            */
            inline static uint32_t GetAvailable(VDP2::VramBank bank)
            {
                return (int)currentTop[(uint16_t)bank] - (int)currentBot[(uint16_t)bank];
            }

            /** @brief Linearly Allocates Vram in a bank and returns address to start of allocation. Allocation fails if
            * there is not enough free space in the bank or if access requires too many cycles.
            * @param size Number of bytes to allocate
            * @param boundary Byte Boundary that the allocation should be aligned to (must be multiple of 32 for all VDP2 Data types)
            * @param bank The VRAM bank to allocate in
            * @param cycles (Optional) Number of Bank Cycles this data will require to access during frame(0-8).
            * @return void* start of the Allocated region in VRAM (nullptr if allocation failed)
            * @note  Any VRAM padded to maintain alignment to a requested boundary is rendered inaccessible to further
            * allocations until VRAM is cleared and reset.
            */
            inline static void* Allocate(uint32_t size, uint32_t boundary, VDP2::VramBank bank, uint8_t cycles = 0)
            {
                void* myAddress = nullptr;

                // Ensure allocation is aligned to requested VRAM boundary:
                uint32_t addrOffset = 0;

                if ((uint32_t)VRAM::currentBot[(uint16_t)bank] & (boundary - 1))
                {
                    addrOffset = boundary - ((uint32_t)currentBot[(uint16_t)bank] & (boundary - 1));
                }

                if (VDP2::VRAM::GetAvailable(bank) >= size + addrOffset)
                {
                    if ((VRAM::bankCycles[(uint16_t)bank] + cycles) < 8)
                    {
                        myAddress = VRAM::currentBot[(uint16_t)bank] + addrOffset;
                        VRAM::currentBot[(uint16_t)bank] += size + addrOffset;
                        VRAM::bankCycles[(uint16_t)bank] += cycles;
                    }
                }

                return myAddress;
            }

            /** @brief Automatically allocates cell data for specified screen
             * @param info Tile cell data description
             * @param screen The screen identifier
             * @return Pointer to the allocated memory
             */
            inline static void* AutoAllocateCell(Tilemap::TilemapInfo& info, uint16_t screen)
            {
                void* alloc;

                if (screen == scnRBG0) // Reserve all 8 cycles of a bank
                {
                    alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A0, 8);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A1, 8);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B0, 8);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B1, 8);
                    if (alloc == nullptr) SRL::Debug::Assert("RBG Cell Allocation failed: insufficient VRAM");
                }
                else // Base cycle requirement on color type
                {
                    uint8_t reqCycles = 0;

                    switch (info.ColorMode)
                    {
                    case CRAM::TextureColorMode::Paletted16:
                        reqCycles = 1;
                        break;
                    case CRAM::TextureColorMode::Paletted256:
                        reqCycles = 2;
                        break;
                    case CRAM::TextureColorMode::RGB555:
                        reqCycles = 4;
                        break;
                    }

                    alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B0, reqCycles);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A1, reqCycles);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A0, reqCycles);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B1, reqCycles);
                    if (alloc == nullptr) SRL::Debug::Assert("NBG Cell Allocation failed: insufficient VRAM");
                }

                return alloc;
            }

            /** @brief Automatically allocates map data for specified screen
             * @param info Tile map data description
             * @param screen The screen identifier
             * @param size optional pointer to pass the resulting allocation size back to
             * @return Pointer to the allocated memory
             */
            inline static void* AutoAllocateMap(Tilemap::TilemapInfo& info, int16_t screen,int* size = nullptr )
            {
                void* alloc = nullptr;
                uint32_t page_sz = 0x800;
                uint32_t sz = (info.MapHeight * info.MapWidth) << 1;

                if (info.CharSize == CHAR_SIZE_1x1) page_sz <<= 2;

                if (info.MapMode == PNB_2WORD)
                {
                    sz <<= 1;
                    page_sz <<= 1;
                }

                if (info.PlaneSize == PL_SIZE_2x2) page_sz <<= 2;
                else if (info.PlaneSize == PL_SIZE_2x1) page_sz <<= 1;

                if (screen == scnRBG0) // Reserve all 8 cycles of bank 0 
                {
                    alloc = VRAM::Allocate(sz, page_sz, VramBank::A0, 8);
                    if (alloc == nullptr) Debug::Assert("RBG Map Allocation failed: insufficient VRAM");
                    else if(size!=nullptr)*size = sz;
                }
                else // Reserve 1 cycle in bank B1 (or B0 if it doesn't conflict with RBG0 map)
                {
                    if (bankCycles[0]!=7) alloc = VRAM::Allocate(sz, page_sz, VramBank::A0, 1);
                    if(!alloc) alloc = VRAM::Allocate(sz, page_sz, VramBank::B1, 1);
                    if (alloc == nullptr) SRL::Debug::Assert("NBG Map Allocation failed: insufficient VRAM");
                    else if(size!=nullptr)*size = sz;
                }

                return alloc;
            }
        };

        /** @brief Bitfield recording all Currently enabled Scroll Screens*/
        inline static uint16_t ActiveScrolls =  NBG3ON| SPRON;

        /** @brief Bitfield recording all Scroll Screens with VDP2 Color Calculation enabled
         */
        inline static uint16_t ColorCalcScrolls =  NBG3ON | SPRON;
        
        /** @brief Bitfield recording all Scroll Screens using Color Offset A
         */
        inline static uint16_t OffsetAScrolls = NBG3ON;
        
        /** @brief Bitfield recording all Scroll Screens using Color Offset B
          */
        inline static uint16_t OffsetBScrolls =  NBG3ON;
        
        /** @brief Bitfield recording all Scroll Screens That Disable transparent pixels
          */
        inline static uint16_t TransparentScrolls = 0;

        /** @brief Functionality available to all Scroll Screen interfaces
         */
        template<class ScreenType, int16_t Id, uint16_t On>
        class ScrollScreen
        {
        private:
            /** @brief Initializes the default tiling pattern of the 4 Planes of the Scroll Screen Depending on the
            * height and width of the source Tilemap.
            * @details The following Tiling Patterns Can Occur:
            *     When Map Data fills area of 2x2 planes or greater
            *         |0|1|
            *         |2|3|
            *     When Map data fits within area of 2x1 planes
            *         |0|1|
            *         |0|1|
            *     When map Data fits within area of 1x2 planes
            *         |0|0|
            *         |1|1|
            *     When map data fits within area of 1 plane
            *         |0|0|
            *         |0|0|
            * @note underlying page Size of the 4 planes is specified separately in TileMapInfo::PlaneSize
            */
            inline static void SetPlanesDefault(Tilemap::TilemapInfo& info)
            {
                int mapX, mapY;
                mapX = mapY = 32;

                if (info.CharSize == CHAR_SIZE_1x1)
                {
                    mapY <<= 1;
                    mapX <<= 1;
                }

                if (info.PlaneSize == PL_SIZE_2x2)
                {
                    mapY <<= 1;
                    mapX <<= 1;
                }
                else if (info.PlaneSize == PL_SIZE_2x1)
                {
                    mapX <<= 1;
                }

                if (info.MapWidth > mapX)
                {
                    if (info.MapHeight > mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 2, 3);
                    else VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 0, 1);

                }
                else
                {
                    if (info.MapHeight > mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 0, 1, 1);
                    else VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 0, 0, 0);
                }
            }

        public:
            /** @brief SGL identifier for the ScrollScreen
             */
            static constexpr int16_t ScreenID = Id;

            /** @brief SGL flag for the ScrollScreen
             */
            static constexpr uint16_t ScreenON = On;

            /** @brief VRAM Address to start of Cell data when displying Tilemap or Bitmap data when using bitmaps
             */
            inline static void* CellAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief  VRAM Address  of ScrollScreen's Map data when displaying Tilemap
             */
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief CRAM Pallet for the ScrollScreen
             */
            inline static CRAM::Palette TilePalette = CRAM::Palette();

            /** @brief Tilemap config info for the ScrollScreen
             */
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();

            /** Size of manually allocated VRAM for Cell Data
             */
            inline static int CellAllocSize = -1;

            /** @brief Size of manually allocated VRAM for Map Data
             */
            inline static int MapAllocSize = -1;

            /** @brief Loads Tilemap data to VRAM using ITilemap Interface and configures the Scroll Screen to use it
             *
             * @details If VRAM for this ScrollScreen's data has already been allocated by the user, SRL will attempt to load
             * to the allocated VRAM and raise assert if the Tilemap Data does not fit within the existing allocation.
             * If VRAM was not allocated SRL will attempt to auto allocate the Tilemap data and raise assert
             * if there is not enough VRAM/cycles available to allocate.
             *
             * @param tilemap The Tilemap to load
             * @note Manual VRAM allocation is for advanced use cases and is NOT verified for proper bank alignment.
             * @note Does not turn Scroll Display on- once loaded use ScrollEnable() to display a Scroll Screen.
             * @note As RBG0 must reserve dedicated VRAM banks always perform loading/allocation 
             * for RBG0 before NBG0-3 screens if using it.
             */
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& tilemap)
            {
                SRL::Tilemap::TilemapInfo myInfo = tilemap.GetInfo();
                ScreenType::Info = tilemap.GetInfo();

                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0)
                {
                    ScreenType::MapAddress = VRAM::AutoAllocateMap(myInfo, ScreenType::ScreenID);
                    if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return;

                }
                else if (ScreenType::MapAllocSize < (ScreenType::Info.MapWidth * ScreenType::Info.MapHeight) << (1+!ScreenType::Info.MapMode))
                {
                    SRL::Debug::Assert("Tilemap Load Failed- MAP DATA exceeds existing VRAM allocation");
                    return;
                }
                
                if ((uint32_t)ScreenType::CellAddress < VDP2_VRAM_A0)
                {
                    ScreenType::CellAddress = VRAM::AutoAllocateCell(myInfo, ScreenType::ScreenID);

                    if ((uint32_t)ScreenType::CellAddress < VDP2_VRAM_A0)
                    {
                        SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                        return;
                    }
                }
                else if (ScreenType::CellAllocSize < ScreenType::Info.CellByteSize)
                {
                    SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                    return;
                }

                int colorID = 0;

                if (ScreenType::Info.ColorMode != SRL::CRAM::TextureColorMode::RGB555)
                {
                    if(ScreenType::TilePalette.GetData()==nullptr)
                    {
                        if ((colorID = SRL::CRAM::GetFreeBank(ScreenType::Info.ColorMode)) < 0)
                        {
                            SRL::Debug::Assert("Tilemap Palette Load Failed- no CRAM Palettes available");
                            return;
                        }
                        
                        SRL::CRAM::SetBankUsedState(colorID, ScreenType::Info.ColorMode, true);
                        ScreenType::TilePalette = SRL::CRAM::Palette(ScreenType::Info.ColorMode, colorID);      
                    }
                    uint16_t len = (ScreenType::Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16) ? 16 : 256;
                    ScreenType::TilePalette.Load((Types::HighColor*)tilemap.GetPalData(), len);
                }

                if (ScreenType::ScreenID != scnRBG0) VDP2::ScrollScreen<ScreenType, Id, On>::SetPlanesDefault(ScreenType::Info);

                VDP2::ScrollScreen<ScreenType, Id, On>::Cell2VRAM((uint8_t*)tilemap.GetCellData(), ScreenType::CellAddress, ScreenType::Info.CellByteSize);
                VDP2::ScrollScreen<ScreenType, Id, On>::Map2VRAM(
                    ScreenType::Info,
                    (uint16_t*)tilemap.GetMapData(),
                    ScreenType::MapAddress,
                    ScreenType::TilePalette.GetId(),
                    VDP2::ScrollScreen<ScreenType, Id, On>::GetCellOffset(ScreenType::Info, ScreenType::CellAddress));
                ScreenType::Init(ScreenType::Info);
            }

            /** @brief Manually Sets VRAM area for Cell Data (Advanced Use Cases)
             * @details This function manually sets an area in VRAM for a scrolls Cell Data to be loaded to. Unless the
             * Address is obtained from VDP2::VRAM::Allocate(), the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data alignment or cycle conflicts. For advanced use cases only.
             * @code {.cpp}
             * //Manually Set NBG0 to store 16bpp Cell Data in an 0x8000 byte region allocated in VRAM bank A1:             * 
             * SRL::VDP2::NBG0::SetCellAddress(SRL::VDP2::VRAM::Allocate(0x8000,32,SRL::VDP2::VramBank::A1, 3),0x8000);
             * @endcode
             * @param address the VRAM address of the allocation
             * @param size the size of the allocation
             * @return Echoes Address
             */
            inline static void* SetCellAddress(void* address, int size)
            {
                ScreenType::CellAddress = address;
                ScreenType::CellAllocSize = size;
                return address;
            }

            /** @brief Manually Sets VRAM area for Map Data (Advanced Use Cases)
             * @details This function manually sets an area in VRAM for a scroll's Map Data to be loaded to. Unless the
             * Address is obtained from VDP2::VRAM::Allocate() the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data alignment or cycle conflicts. For advanced use cases only.
             * @code {.cpp}
             * //Manually Set NBG0 to store Map Data in an 0x8000 byte region allocated in VRAM bank A1:
             * SRL::VDP2::NBG0::SetMapAddress(SRL::VDP2::VRAM::Allocate(0x8000,1024,SRL::VDP2::VramBank::A1, 1),0x8000);
             * @endcode
             * @param Address the VRAM address of the allocation
             * @param Size the size of the allocation
             * @return Echoes Address
             */
            inline static void* SetMapAddress(void* Address, int Size)
            {
                ScreenType::MapAddress = Address;
                ScreenType::MapAllocSize = Size;
                return Address;
            }

            /** @brief Registers Scroll in VDP2 cycle pattern to enable display of this Scroll Screen
             * @details Asserts when registration of a scroll fails due to cycle pattern conflicts.
             * Possible causes:
             *     -Too many Scroll Layers with higher bit depth are storing data in the same VRAM bank
             *     -NBG0 or NBG1 have their minimum scale limit set too small(eg 1/2x or 1/4x scale)
             *     -NBG Data was stored in a bank reserved by RBG0
             * Potential conflicts are minimized when using Automatic Allocation and setting the
             * desired scale limits of NBG0/NBG1 beforehand.
             * @note Even when registration is successful, some scrolls may be unable to  display simultaneously
             * when the color depth of NBG0 or NBG1 is too High:
             *        -When NBG0 > 8bpp, NBG2 will not display
             *        -When NBG1 > 8bpp, NBG3 will not display
             * By default, debug ASCII text is displayed on NBG3, so will not be available if NBG1 is 
             * displayed in a high color mode.  
             */
            inline static void ScrollEnable()
            {
                VDP2::ActiveScrolls |= ScreenType::ScreenON;
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }

            /** @brief Removes the Scroll Screen from VDP2 cycle pattern register to disable display
             * @details Asserts when registration of a scroll fails due to cycle pattern conflicts.
             * Possible causes:
             * Assert should never occur here unless the user independently invoked SGL's slScrAutoDisp() with
             * specifications differing from those tracked by SRL
             */
            inline static void ScrollDisable()
            {
                VDP2::ActiveScrolls &= ~(ScreenType::ScreenON);
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }
          
            /** @brief Gets the starting address in VRAM of Map data allocated to this scroll
             * @return Address of Map data
             */
            inline static  void* GetMapAddress() { return ScreenType::MapAddress; }

            /** @brief Gets the starting address in VRAM of Cell data allocated to this scroll
             *  @return Address of Cell data
             *  @note Returns starting address of bitmap when displaying Bitmap Scroll
             */
            inline static  void* GetCellAddress() { return ScreenType::CellAddress; }

            /** @brief returns the VRAM Address of the specified page number in a scroll's Page Table
             * or nullptr if the requested page is outside of allocated Map Bounds
             * @param index The index of desired page in the page table
             * @return VRAM Address of the page index
             */

            inline static  void* GetPageAddress(uint8_t index)
            {
                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return nullptr;
                uint32_t offset = 2048 * (uint32_t)index;
                if (ScreenType::Info.CharSize == CHAR_SIZE_1x1) offset <<= 2;
                if (ScreenType::Info.MapMode == PNB_2WORD) offset <<= 1;
                return ScreenType::MapAddress + (void*)offset;
            }

            /** @brief Gets the VRAM Address of the specified plane in a scroll's Page Table (units of 1x1, 2x1, or 2x2 pages)
             * @param index The index of desired plane in page table
             * @return VRAM Address of the plane index
             * @note PlaneAddress and PageAddress are equal when Plane Size is set to 1x1
             */
            inline static  void* GetPlaneAddress(uint8_t index)
            {
                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return nullptr;
                uint32_t offset = 2048 * (uint32_t)index;
                if (ScreenType::Info.CharSize == CHAR_SIZE_1x1) offset <<= 2;
                if (ScreenType::Info.MapMode == PNB_2WORD) offset <<= 1;
                if (ScreenType::Info.PlaneSize == PL_SIZE_2x2)offset <<= 2;
                else if (ScreenType::Info.PlaneSize == PL_SIZE_2x1)offset <<= 1;
                return (void*)((uint32_t)ScreenType::MapAddress + offset);
            }

            /** @brief Manually set the Plane layout of a Scroll Screen
             * @details This function manually sets the 4 planes comprising a NBG scroll screen
             * in cases when the default plane tiling pattern is not desired.
             * @param a,b,c,d the plane indicies of the 4 planes that will display in the normal scroll
             * @note RBG0 does not currently support multi plane patterns, so only plane [a] will be used
             */
            inline static void SetMapLayout(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            {
                ScreenType::SetPlanes(
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(a),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(b),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(c),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(d));
            }

            /** @brief Set the opacity of a scroll screen
             *  @details This Function takes the opacity specified as a fixed point value and converts it to
             *  one of the 32 color calculation ratios that a scroll screen can use (value is floored to the nearest ratio).
             *  Color Calculation is turned on if Opacity < 1.0, or off if Opacity>= 1.0. Color calculation is unchanged if value is negative.
             *  @note Color ratios only apply to highest priority pixels in frame
             *  @note When VDP2 ColorCalcMode is set to UseColorAddition, all scrolls with opacity < 1.0 will use color addition
             *  in place of their specified ratios.
             *  @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity for the scroll screen (Default 1.0)
             */
            inline static void SetOpacity(Math::Fxp opacity = 1.0)
            {
                if (opacity < Math::Fxp(0.0)) return;

                else if (opacity >= Math::Fxp(1.0))
                {
                    VDP2::ColorCalcScrolls &= ~(ScreenType::ScreenON);
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
                else
                {
                    slColRate(ScreenType::ScreenID, 31 - (uint16_t)(opacity.RawValue() >> 11));
                    VDP2::ColorCalcScrolls |= ScreenType::ScreenON;
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
            }

            /** @brief Set the Display Priority of a Scroll Screen
             * @note Higher value layers display on top of lower layers.
             * @note When 2 or more scroll screens are assigned the same layer, their priority resolves as
             * SPRITE>RBG0>NBG0>NBG1>NBG2>NBG3.
             * @param pr The Priority Layer for the Scroll Screen
             */
            inline static void SetPriority(SRL::VDP2::Priority pr) { slPriority(ScreenType::ScreenID, (uint16_t)pr); }

            /** @brief Sets Which Color Offset that a scroll Screen should use
            *  @details Scroll Screens can optionally apply one of 2 registered RGB color offsets
            *  to their pixels at the end of VDP2 processing- either Offset A or Offset B. 
            *  The values of the offsets for each RGB channel are set with VDP2::SetOffsetA() and VDP2::SetOffsetB().
            *  Use this function to enable a scroll to use one of these offsets.
            *  @param mode the color offset to use for this scroll 
            *  @note Because the color offset is applied at the end of VDP2 pipeline, only top priority pixels are affected.
            */
            inline static void UseColorOffset(VDP2::OffsetChannel mode)
            {
                if (mode == OffsetChannel::OffsetA)
                {
                    VDP2::OffsetAScrolls |= ScreenType::ScreenON;
                    OffsetBScrolls &= ~ ScreenType::ScreenON;
                }
                else if (mode == OffsetChannel::OffsetB)
                {
                    OffsetBScrolls |= ScreenType::ScreenON;
                    OffsetAScrolls &= ~ScreenType::ScreenON;
                }
                else
                {
                    OffsetAScrolls &= ~ScreenType::ScreenON;
                    OffsetBScrolls &= ~ScreenType::ScreenON;
                }

                slColOffsetOn(0);//clear all offsets  
                slColOffsetAUse(OffsetAScrolls);//re register offsets for A 
                slColOffsetBUse(OffsetBScrolls);//re register offsets for B
            }

            /** @brief Enable transparent pixels for a scroll screen
            *   @details When enabled any pixel data that is 0 (regardless of bit depth)
            *   will be treated as transparent and display the layer behind it.
            *   @note Transparent pixels are enabled for all Scroll Screens by default
            */
            inline static void TransparentEnable()
            {
                VDP2::TransparentScrolls &= ~ScreenType::ScreenON;
                //SGLs naming convention is reversed- flagging ScreenON turns OFF transparency
                slScrTransparent(VDP2::TransparentScrolls);
            }

            /** @brief Disable transparent pixels for a scroll screen
            *   @details When disabled any pixel data that is 0 (regardless of bit depth)
            *   will use the color from index 0 in its CRAM pallet, or black if RGB. 
            *   @note Transparent pixels are enabled for all Scroll Screens by default.
            */
            inline static void TransparentDisable()
            {    
                VDP2::TransparentScrolls |= ScreenType::ScreenON;
                //SGLs naming convention is reversed- flagging ScreenON turns OFF transparency
                slScrTransparent(VDP2::TransparentScrolls);
            }

            /** @brief Compute the offset that must be added to map data When Corresponding Cell Data does not start on a VRAM bank boundary
            * @param tile The data configuration of the tilemap
            * @param cellAddress Address of corresponding Cell Data in VRAM (must be a 32 byte boundary)
            * @return The Cell Offset to add to to map data
            */
            inline static uint32_t GetCellOffset(SRL::Tilemap::TilemapInfo& tile, void* cellAddress)
            {
                uint32_t cellOffset;

                if (!ScreenType::Info.MapMode) cellOffset = ((uint32_t)cellAddress - VDP2_VRAM_A0) >> 5; // 2WORD
                else if (ScreenType::Info.MapMode == 0x8000) // 1WORD MODE 0
                {
                    if (ScreenType::Info.CharSize) cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 7; // 2x2
                    else cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x7FFF) >> 5; // 1x1
                }
                else // 1WORD MODE 1
                {
                    if (ScreenType::Info.CharSize) cellOffset = ((uint32_t)cellAddress - VDP2_VRAM_A0) >> 7; // 2x2
                    else cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 5; // 1x1
                }

                return cellOffset;
            }



            /** @brief Gets the Pallet Bank That must be included in Map Data to Reference a Palette in CRAM
             * @param paletteID (optional) specify to reference an arbitrary palette, otherwise uses Id from ScrollScreen::TilePalette
             * @return The Formatted Palette ID to be included in Map Indicies to reference a specified palette
             */
            inline static uint32_t GetPalOffset(int8_t paletteID = -1)
            {
                uint32_t paletteOffset;

                if (paletteID < 0) paletteID = ScreenType::TilePalette.GetId();

                if (ScreenType::Info.MapMode) paletteOffset = paletteID << 12;
                else paletteOffset = paletteID << 24;

                return paletteOffset;
            }
        private:

            /** @brief Copies Cell data to VRAM (adapted from SGL samples).
            * @param CellData Cell Data to copy.
            * @param CellAdr VRAM address to copy to.
            * @param Size Number of bytes to copy.
            */
            inline static void Cell2VRAM(uint8_t* cellData, void* cellAdr, uint32_t size)
            {
                uint8_t* VRAM = (uint8_t*)cellAdr;

                // Note: Consider DMA
                for (uint32_t i = 0; i < size; i++) *(VRAM++) = *(cellData++);
            }

            /** @brief Copies map data to VRAM and applies necessary offsets (adapted from SGL Samples).
             * @param info Tilemap data config.
             * @param mapData Map data to copy to VRAM.
             * @param mapAdr VRAM address to copy map to .
             * @param mapoff offset added when Cell data does not start at bank boundary .
             * @param paloff Palette index in CRAM.
             */
            inline static void Map2VRAM(SRL::Tilemap::TilemapInfo& info, uint16_t* mapData, void* mapAdr, uint8_t paloff, uint32_t mapoff)
            {
                uint16_t* VRAM = (uint16_t*)mapAdr;
                uint32_t* VRAM32 = (uint32_t*)mapAdr;
                uint32_t* Data32 = (uint32_t*)mapData;

                for (uint16_t i = 0; i < info.MapHeight; i++)
                {
                    for (uint16_t j = 0; j < info.MapWidth; j++)
                    {
                        if (info.MapMode) *VRAM++ = ((*mapData++) + mapoff) | (paloff << 12); // 1WORD data
                        else *VRAM32++ = ((*Data32++) + mapoff) | (paloff << 20); // 2WORD data
                    }
                }
            }

        };

        /** @brief NBG0 interface
         *  @details Normal Background Scroll 0:
         *      -Available color depths: Paletted16, Paletted256, RGB555
         *      -Available Modes: Bitmap, Tilemap
         *      -Available features: Vertical/Horizontal Scrolling, Scaling, LineScroll Tables
         */
        class NBG0 : public ScrollScreen<NBG0, scnNBG0, NBG0ON>
        {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg0(info.SGLColorMode(), info.CharSize);
                slPageNbg0(NBG0::CellAddress, (void*)NBG0::TilePalette.GetData(), info.MapMode);
                slPlaneNbg0(info.PlaneSize);
                slMapNbg0(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg0(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg0(pos.X.RawValue(), pos.Y.RawValue()); }

            /** @brief Sets the Scale of NBG0 Screen display
             *  @param scl Fixed Point X an Y scaling values
             *  @note The minimum scale that can be displayed is determined by The Scale Limit, Scaling values
             *  lower than the minimum will be clamped to the minimum
             */
            static void SetScale(Math::Vector2D& scl) { slScrScaleNbg0(scl.X.RawValue(), scl.Y.RawValue()); }
        };

        /** @brief NBG1 interface
         * @details Normal Background Scroll 1:
         *      -Available color depths: Paletted16, Paletted256, RGB555
         *      -Available Modes: Bitmap, Tilemap
         *      -Available features: Vertical/Horizontal Scrolling, Scaling, LineScroll Tables
         */
        class NBG1 : public ScrollScreen<NBG1, scnNBG1, NBG1ON>
        {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg1(info.SGLColorMode(), info.CharSize);
                slPageNbg1(NBG1::CellAddress, (void*)NBG1::TilePalette.GetData(), info.MapMode);
                slPlaneNbg1(info.PlaneSize);
                slMapNbg1(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg1(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg1(pos.X.RawValue(), pos.Y.RawValue()); }

            /** @brief Sets the Scale of NBG1 Screen display
             * @param scl Fixed Point X an Y scaling values
             * @note The minimum scale that can be displayed is determined by The Scale Limit, Scaling values
             * lower than the minimum will be clamped to the minimum
             */
            static void SetScale(Math::Vector2D& scl) { slScrScaleNbg1(scl.X.RawValue(), scl.Y.RawValue()); }
        };

        /** @brief  NBG2 interface
         *  @details Normal Background Scroll 2:
         *      -Available color depths: Paletted16, Paletted256
         *      -Available Modes: Tilemap
         *      -Available features: Vertical/Horizontal Scrolling
         *  @note Unavailable when NBG0 color mode is RGB555   
         */
        class NBG2 : public ScrollScreen<NBG2, scnNBG2, NBG2ON>
        {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg2(info.SGLColorMode(), info.CharSize);
                slPageNbg2(NBG2::CellAddress, (void*)NBG2::TilePalette.GetData(), info.MapMode);
                slPlaneNbg2(info.PlaneSize);
                slMapNbg2(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg2(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg2(pos.X.RawValue(), pos.Y.RawValue()); }
        };

        /** @brief NBG3 interface
         *  @details Normal Background Scroll 3:
         *      -Available color depths: Paletted16, Paletted256
         *      -Available Modes: Tilemap
         *      -Available features: Vertical/Horizontal Scrolling
         *  @note Unavailable when NBG0 color mode is RGB555.
         *  @note SRL Uses to dispay Degbug Text by default.    
         */
        class NBG3 : public ScrollScreen<NBG3, scnNBG3, NBG3ON>
        {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg3(info.SGLColorMode(), info.CharSize);
                slPageNbg3(NBG3::CellAddress, (void*)NBG3::TilePalette.GetData(), info.MapMode);
                slPlaneNbg3(info.PlaneSize);
                slMapNbg3(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg3(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y  Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg3(pos.X.RawValue(), pos.Y.RawValue()); }
        };

        /** @brief setting for RBG0,1 rotation constraints
         * @note more axis require more VRAM resources
         */
        enum class RotationMode
        {
            /** @brief 2d rotation with only roll and zoom
             * @note No additional VRAM requirements
             */
            OneAxis,

            /** @brief 3d rotation with pitch and yaw, but no roll (modified per line)
             * @note Requires 0x2000-0x18000 bytes in arbitrary VRAM Bank (No cycles)
             */
            TwoAxis,

            /** @brief Full 3d rotation with pitch, yaw and roll (modified per pixel)
             * @note Requires 0x2000-0x18000 bytes in Reserved VRAM bank (8 cycles)
             */
            ThreeAxis,
        };

        /** @brief RBG0 interface
         * @details Rotating Background Scroll 0:
         *      -Available color depths: Paletted16, Paletted256, RBG555
         *      -Available modes: Tilemap, Bitmap
         *      -Available features: Scrolling, Scaling, Rotation, 
         *      -Coefficient table allows per line and per pixel scaling
         *       to simulate perspective
         */
        class RBG0 : public ScrollScreen<RBG0, scnRBG0, RBG0ON>
        {
        public:
            /** @brief VRAM Address of RBG0 Coefficient table
             */
            inline static void* KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            inline static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                //slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));
                slRparaMode(RA);
                slOverRA(0);
                slCharRbg0(info.SGLColorMode(), info.CharSize);
                slPageRbg0(CellAddress, (void*)RBG0::TilePalette.GetData(), info.MapMode);
                slPlaneRA(info.PlaneSize);
                sl1MapRA(MapAddress);
                slPopMatrix();
            }

            /** @brief Select what type of rotation to use for the rotating scroll (Call before Loading RBG0)
             * @param mode The RotationMode to use for this scroll
             * @param vblank Chose to update VRAM at VBLANK to reduce amount of coefficient
             * data required for rotation of a plane (default = true)
             * @note when 2 or 3 axis rotation is Selected, VRAM will be allocated to store
             * necessary coefficient data. If Vblank is set false, all coefficients will be
             * statically stored in VRAM as a 0x18000 byte table. If Vblank is set true, only
             * the coefficients necessary for the current frame will be dynamically written
             * to VRAM at Vblank, reducing VRAM footprint to 0x2000 bytes per
             * rotation parameter but increasing required Vblank overhead.
             */
            inline static void SetRotationMode(VDP2::RotationMode mode, bool vblank = true)
            {
                //slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));

                switch (mode)
                {
                case RotationMode::OneAxis:
                    slKtableRA(nullptr, K_OFF);
                    //VDP2_RAMCTL &= 0xffcf; // Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified
                    break;

                case RotationMode::TwoAxis:
                    if (!vblank)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 0);
                        slMakeKtable((void*)VDP2::RBG0::KtableAddress);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_FIX | K_LINE | K_2WORD | K_ON);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 0);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_LINE | K_2WORD | K_ON);
                    }

                    //VDP2_RAMCTL &= 0xffcf;//Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified
                    break;

                case RotationMode::ThreeAxis:
                    if (!vblank)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 8);
                        slMakeKtable((void*)VDP2::RBG0::KtableAddress);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_FIX | K_DOT | K_2WORD | K_ON);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 8);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_DOT | K_2WORD | K_ON);
                    }

                    break;
                }
            }

            /** @brief Writes the current matrix transform to RBG0RA Rotation parameters
             * to update its position and perspective
             */
            inline static void SetCurrentTransform()
            {
                slCurRpara(RA);
                slPushMatrix();
                {
                    slScrMatConv();
                    slScrMatSet();
                }
                slPopMatrix();
            }

            /** @brief Sets the plane of Tilemap Data to be displayed
             * @param a,b,c,d Page Table addresses of the planes to display
             * @note Multi Plane Maps are not supported yet for RBG0, only plane a is used
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { 
                
                sl1MapRA(a);
            }
            
             /** @brief Sets the plane of Tilemap Data to be displayed with 16 planes
             * @param layout 4x4 array of uint8_t indecies representing the index of each plane in the map layout
             * @note Unlike NBG scrolls RBG0 only loads with the default tiling of 1 plane. Use this
             * function after loading to set the arrangement of multi plane tilemaps within a 4x4 grid.
             * @note No check is performed to ensure the indecies entered are within the bounds of
             * the scroll's map data. Specifying indices larger than the number of planes loaded 
             * will result in the diplay of garbage data in those portions of the map  
             */
            static void SetPlanes(const uint8_t layout[4][4])
            {   
                uint8_t sz = (VDP2::RBG0::Info.CharSize==CHAR_SIZE_1x1) ? 4:1;
                uint8_t sLayout[4][4] = {};
                if (VDP2::RBG0::Info.MapMode == PNB_2WORD) sz <<= 1;
                if (VDP2::RBG0::Info.PlaneSize == PL_SIZE_2x2)sz <<= 2;
                else if (VDP2::RBG0::Info.PlaneSize == PL_SIZE_2x1)sz <<= 1;
                for (size_t i = 0; i < 4; ++i) { 
                    for (size_t j = 0; j < 4; ++j) { 
                        sLayout[i][j] = layout[i][j] * sz;
                    }
                }
                sl16MapRA((uint8_t*)sLayout);
            }
            
        };

        /** @brief Sprite Color Calculation Conditions (See SpriteLayer::SetColorCondition() for details)
         */
        enum class ColorCondition : uint16_t
        {
            /** @brief Perform color calculation only if priority is less than TestValue
             */
            PriorityLessThan = CC_pr_CN,

            /** @brief Perform color calculation only if priority is equal to  TestValue
             */
            PriorityEquals = CC_PR_CN,

            /** @brief Perform color calculation only if priority is greater than TestValue
             */
            PriorityGreaterThan = CC_PR_cn,
        };

        /** @brief Used to reference the Sprite color calculation ratio and priority registers
         * @note Can refer to Color Calculation banks or Priority Banks depending on use
         */
        enum class SpriteBank : int16_t
        {
            Bank0 = scnSPR0,
            Bank1 = scnSPR1,
            Bank2 = scnSPR2,
            Bank3 = scnSPR3,
            Bank4 = scnSPR4,
            Bank5 = scnSPR5,
            Bank6 = scnSPR6,
            Bank7 = scnSPR7,
        };

        /** @brief Interface to control VDP2 settings for the Sprite Layer (data from VDP1 framebuffer),
         * such as Display priority and VDP2 color calculation)
         */
        class SpriteLayer
        {
        public:
            /** @brief Turns VDP2 Color Calculation on for Sprite Layer
             */
            inline static void ColorCalcON()
            {
                VDP2::ColorCalcScrolls |= SPRON;
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }

            /** @brief Turns VDP2 Color Calculation Off for Sprite Layer
             */
            inline static void ColorCalcOFF()
            {
                VDP2::ColorCalcScrolls &= (~SPRON);
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }

            /** @brief Set the opacities that sprites can select from in CC registers
             * @details This Function takes the opacity specified as a fixed point value and converts it to
             * one of the 32 color calculation ratios that the system can use (value is floored to the nearest ratio).
             * It then sets the ratio in the specified sprite cc register (cc register 0 if not specified)
             * @note Color ratios only apply to highest priority pixels in frame
             * @note When Color Calc is ON, max opacity is ~(0.97). Fully opaque sprites must select with color condition
             * @note When ColorCalcMode is set to UseColorAddition, The Opacity levels are ignored and Color addition is applied
             * to all sprites whose priority meets the color condition (see SpriteLayer::SetColorCondition() for details)
             * @note RGB sprites always use the opacity set in CC register[bank0]
             * @note Does NOT turn color calc ON or OFF for the Sprite Layer(use SpriteLayer::ColorCalcON,OFF())
             * @note available cc registers vary by Palette code config- default allows all 8 banks
             * @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity of the cc register
             * @param bank (optional) which of the 8 CC registers to Set the opacity in (defaults to 0)
             */
            inline static void SetOpacity(SRL::Math::Types::Fxp opacity, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                if (opacity < 0.0 || opacity > 1.0)
                {
                    return;
                }
                else
                {
                    slColRate((int16_t)bank, 31 - (uint16_t)(opacity.RawValue() >> 11));
                }
            }

            /** @brief Set the priority Layers That sprites can select from in PR registers
             * @details This function sets one of the 8 priority registers that a Palette sprite can reference (default bank0)
             * @note available registers vary by Palette code config- default can only pull from bank0 and bank1
             * @note RGB sprites always use the priority from bank0
             * @note During VDP2 init, priority bank0 and bank1 are initialized to Layer3 and Layer4 respectively
             * @note Changing these priorities will result in differing behavior for sprite color calculation
             * (See SpriteLayer::SetColorCondition() for more details)
             * @param pr enum VDP2::Priority Layer
             * @param bank (optional) enum VDP2::SpriteBank designating which priority bank to write to
             */
            inline static void SetPriority(SRL::VDP2::Priority pr, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                slPriority((int16_t)bank, (uint16_t)pr);
            }

            /** @brief Set conditions under which VDP2 color calculation is performed on sprites based on their priority.
             * @details Sets up the condition that allows only select sprites to receive Half Transparent
             *  color calculation with VDP2 layers. To make a sprite fully opaque, selectively turn color calculation off for it by
             *  assigning it to use a Priority Bank containing a priority layer that does not satisfy the Color Condition.
             *  The default VDP2 initialization uses ColorCondition::PriorityEquals Priority::Layer4,
             *  with SpriteBank0 set to Layer3 and SpriteBank1 set to Layer4. With this config RGB sprites receive no VDP2
             *  color calculation, while Palette sprites only receive color calculation when they select priority from SpriteBank1
             * @param Condition The type of condition that VDP2 Color Calculation will follow.
             * @param TestValue The Layer that a sprite's priority will be tested against in the condition.
             */
            inline static void SetColorCondition(VDP2::ColorCondition Condition, SRL::VDP2::Priority TestValue)
            {
                slSpriteCCalcCond((uint16_t)Condition);
                slSpriteCCalcNum((uint16_t)TestValue);
            }
        };

        /** @brief Clear all VDP2 VRAM allocations and reset all Scroll Screen VRAM References, as well
         *  as all CRAM allocations associated with VDP2 Scroll Screens
         * @note, When Loading a new set of Data and Configurations for Scroll Screens with auto allocation, Call this first
         *  to ensure old data is freed
         */
        inline static void ClearVRAM()
        {
            //reset ScrollScreen VRAM References
            VDP2::NBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG0::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG0::LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG0::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG0::TilePalette.GetId(), VDP2::NBG0::Info.ColorMode, false);
                VDP2::NBG0::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG1::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG1::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG1::LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG1::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG1::TilePalette.GetId(), VDP2::NBG1::Info.ColorMode, false);
                VDP2::NBG1::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG2::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG2::CellAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG2::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG2::TilePalette.GetId(), VDP2::NBG2::Info.ColorMode, false);
                VDP2::NBG2::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG3::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG3::CellAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG3::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG3::TilePalette.GetId(), VDP2::NBG3::Info.ColorMode, false);
                VDP2::NBG3::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::RBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::RBG0::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::RBG0::TilePalette.GetId(), VDP2::RBG0::Info.ColorMode, false);
                VDP2::RBG0::TilePalette = SRL::CRAM::Palette();
            }

            // Clear VRAM banks
            for (int i = 0; i < 4; ++i)
            {
                VDP2::VRAM::currentTop[i] = VDP2::VRAM::bankTop[i];
                VDP2::VRAM::currentBot[i] = VDP2::VRAM::bankBot[i];
                VDP2::VRAM::bankCycles[i] = -1;
            }
            // Clear Rotation control bits of VDP2_RAMCTL 
            VDP2_RAMCTL &= 0xff00;
            //leave cylces reserved for ASCII 
            VDP2::VRAM::bankCycles[3] = 1;
        }

        /** @brief Set the back color
         * @param color Color to use
         */
        inline static  void SetBackColor(const Types::HighColor& color)
        {
            slBack1ColSet((void*)(VDP2_VRAM_A1 + 0x1fffe), (uint16_t)color);
        }

        /** @brief Set color for print
         * @param index Color index
         * @param color Color to set
         */
        inline static void SetPrintPaletteColor(const uint16_t index, const Types::HighColor& color)
        {
            *(((Types::HighColor*)CRAM::BaseAddress) + (1 + (index << 8))) = (uint16_t)color;
        }

        /** @brief Initialize VDP2
         * @param backColor starting back color
         */
        inline static void Initialize(const Types::HighColor& backColor)
        {
            // Initialize color ram mode
            slColRAMMode(CRM16_2048);

            //slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
            VDP2::SetPrintPaletteColor(0, Types::HighColor::Colors::White);
            VDP2::SetPrintPaletteColor(1, Types::HighColor::Colors::Black);
            VDP2::SetPrintPaletteColor(2, Types::HighColor::Colors::Red);
            VDP2::SetPrintPaletteColor(3, Types::HighColor::Colors::Green);
            VDP2::SetPrintPaletteColor(4, Types::HighColor::Colors::Blue);
            VDP2::SetPrintPaletteColor(5, Types::HighColor::Colors::Yellow);
            VDP2::SetPrintPaletteColor(6, Types::HighColor::Colors::Magenta);

            // Set default color to white
            //slCurColor(0);

            // Set back color
            VDP2::SetBackColor(backColor);

            // Initialize ascii print to use NBG3
            SRL::ASCII::LoadFontSG((uint8_t*)(VDP2_VRAM_B1 + 0x800), 0);
            SRL::ASCII::SetPalette(0);
            int myID = SRL::CRAM::GetFreeBank(SRL::CRAM::TextureColorMode::Paletted16);
            SRL::CRAM::SetBankUsedState(myID, SRL::CRAM::TextureColorMode::Paletted16, true);
            SRL::ASCII::SetFont(0);
            slCharNbg3(COL_TYPE_16, CHAR_SIZE_1x1);
            slPageNbg3((void*)(VDP2_VRAM_B1 + 0x1D000), 0, PNB_1WORD | CN_10BIT);
            slPlaneNbg3(PL_SIZE_1x1);
            slMapNbg3((void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000));
            SRL::VDP2::NBG3::SetPriority(SRL::VDP2::Priority::Layer7);
            SRL::VDP2::NBG3::ScrollEnable();
            SRL::VDP2::NBG0::ScrollDisable(); // We don't want this on by default anymore
            // Fix param table at top of VRAM outside range of allocator - now user calls to slPerspective will always update here:
            slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_A0 + 0x1ff00));
        }

        /** @brief data strUcture of a VDP2 color offset to be set in Offset A or Offset B
         *  @details The offset data that will be set is a signed 9 bit value per color channel.
         *  The valid range of inputs is -255 to +255. The sign determines whether the color offset 
         *  is additive or subtractive. Values outside the range will be clamped to it when
         *  the offset is set. See SetColorOffsetA and SetColorOffsetB for more details. 
         */
        /** @brief Data structure of a VDP2 color offset to be set in Offset A or Offset B
         *  @details The offset data that will be set is a signed 9 bit value per color channel.
         *  The valid range of inputs is -255 to +255. The sign determines whether the color offset 
         *  is additive or subtractive. Values outside the range will be clamped on initialization.
         *  See SetColorOffsetA and SetColorOffsetB for more details. 
         */
        struct ColorOffset
        {
            /** @brief  Red channel offset/
             */ 
            int16_t Red;

            /** @brief  Green channel offset/
             */
            int16_t Green;

            /** @brief  Blue channel offset/
             */
            int16_t Blue;
            
            /** @brief Initialize with all channel offsets set to 0 (No Offset)
             */
            ColorOffset()
            {
                Red = 0;
                Green = 0;
                Blue = 0;
            }

            /** @brief initialize red green and blue channel offsets
             *  @param red offset for red channel
             *  @param green offset for green channel
             *  @param blue offset for blue channel
             */
            ColorOffset(int16_t red, int16_t green, int16_t blue)
                : Red(SRL::Math::Clamp<int16_t>(red, -255, 255)),
                  Green(SRL::Math::Clamp<int16_t>(green, -255, 255)),
                  Blue(SRL::Math::Clamp<int16_t>(blue, -255, 255))
            {
            }
            
            /** @brief initialize from an RGB555 source color
             *  @param col source HighColor to initialize from
             *  @note Can only initialize positive offsets. Use in conjunction with
             *  -= operator to subtract.
             */
            ColorOffset(const SRL::Types::HighColor& col)
            {
                this->Red = col.Red*8;
                this->Green = col.Green*8;
                this->Blue = col.Blue*8;
            }
            
            /** @brief Set this offset equal to another
             *  @param col offset to set
             */
            constexpr ColorOffset& operator=(const ColorOffset& col)
            {
                Red = col.Red;
                Green = col.Green;
                Blue = col.Blue;
                return *this;
            }

            /** @brief Add to this offset
             *  @param col the offset to add
             */
            constexpr ColorOffset& operator+=(const ColorOffset& col)
            {
                Red = SRL::Math::Clamp<int16_t>(Red+col.Red, -255, 255);
                Green =SRL::Math::Clamp<int16_t>(Green+col.Green,-255,255);
                Blue = SRL::Math::Clamp<int16_t>( Blue+col.Blue,-255, 255);
                return *this;
            }
            /** @brief Subtract from this offset
             *  @param col the offset to subtract
             */
            constexpr ColorOffset& operator-=(const ColorOffset& col)
            {
                Red = SRL::Math::Clamp<int16_t>(Red-col.Red, -255, 255);
                Green =SRL::Math::Clamp<int16_t>(Green-col.Green, -255, 255);
                Blue = SRL::Math::Clamp<int16_t>(Blue-col.Blue, -255, 255);
                return *this;
            }

        };

        /** @brief Sets RGB color channel offset A
         *  @details VDP2 supports 2 RGB color offsets stored in special registers 
         *  that any ScrollScreen or SpriteLayer can use. When registered all non-transparent
         *  pixels from the scroll screen will have the offset applied after all other color
         *  calculations are performed.
         *  @param offset The color offset to apply with Offset A
         */
        static void SetColorOffsetA(VDP2::ColorOffset & offset) //int16_t red, int16_t green, int16_t blue)
        {
            slColOffsetA(offset.Red, offset.Green, offset.Blue);
        }

        /** @brief Sets RGB color channel offset B
         *  @details VDP2 supports 2 RGB color offsets stored in special registers
         *  that any ScrollScreen or SpriteLayer can use. When registered all non-transparent
         *  pixels from the scroll screen will have the offset applied after all other color
         *  calculations are performed.
         *  @param offset The color offset to apply with Offset B
         */
        static void SetColorOffsetB(VDP2::ColorOffset & offset) 
        {
            slColOffsetB(offset.Red, offset.Green, offset.Blue);
        }

        /** @brief Basic Options for behavior of VDP2 Half Transparent Color Calculation
         */
        enum class ColorCalcMode : uint16_t
        {
            /** @brief Blend colors by the ratio specified in Higher priority Pixels
             */
            UseColorRatiosTop = 0x000,

            /** @brief Blend colors by the ratio specified in Lower priority Pixels
             */
            UseColorRatios2nd = 0x200,

            /** @brief Use additive color blending instead of ratios
             */
            UseColorAddition = 0x100,
        };
      
        /** @brief Sets VDP2 Half Transparent Color Calculation Mode (only one mode can be used at once)
         * @param mode The VDP2 color calculation mode to use
         * @param extend Designates whether to extend color calculation to the top 3 Layer Priories instead of just top 2
         * @note Extended color calculation has many restrictions detailed in VDP2 users manual- not all color modes can support
         * extension simultaneously. If supported, 3rd priority pixels will blend with 2nd priority at a 50:50 ratio before the result is 
         * blended with top priority pixels. If unsupported the behavior is identical to non-extended color calculation. 
         */
        inline static void SetColorCalcMode(VDP2::ColorCalcMode mode = VDP2::ColorCalcMode::UseColorRatiosTop, bool extend = false)
        {
            uint16_t flags = (uint16_t)mode;
            if(extend) flags |= CC_EXT;
            slColorCalc((uint16_t)flags);
        }
    };
}
