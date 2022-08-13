#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <array>
#include <string>
#include <string_view>

#include <limits>


#include "opcode.h"

/*
 PC:
 -Address bus output enable
 -Address bus input enable
 -Increment next instruction
 -Enable
 -Load
 
 
 ALU:
 -Input enable
 -Output enable
 -Write output
 -Write alu A
 -Write alu B
 -Write alu F
 
 LSU:
 -Increment stack pointer
 -Decrement stack pointer
 -Read only memory enable
 -Random access memory enable
 -Read enable
 -Write enable
 -Flags output enable
 
 EAU:
 -Effective address high write
 -Effective address low write
 -Data bus input enable
 -Address bus output enable
 
 RF:
 -F register output enable
 -D register output enable
 -C register output enable
 -B register output enable
 -A register output enable
 -F register input enable
 -D register input enable
 -C register input enable
 -B register input enable
 -A register input enable
 
 
 
 */

#define BIT(x) (1ull << x##ull)
#define IMM(x) (x << 56ull)


enum : std::uint64_t
{
    IMMEDIATE_IN = BIT(55),
    

    
    JUMP_GREATER_THAN_ZERO = BIT(45),
    JUMP_CARRY_SET         = BIT(44),
    JUMP_ON_ZERO           = BIT(43),
    
    PCU_INSTRUCTION_REGISTER_IN = BIT(42),
    
    RF_TEMP_OUT = BIT(41),
    RF_TEMP_IN  = BIT(40),
    
    CONNECT_F_TO_DATA_BUS = BIT(39),
    
    PC_LOAD_VALUE               = BIT(38),
    PC_ENABLE                   = BIT(37), //should be unused
    PC_SKIP_TO_NEXT_INSTRUCTION = BIT(36),
    PC_ADDRESS_BUS_OUT          = BIT(35),
    PC_ADDRESS_BUS_IN           = BIT(34),

    EAU_ADDRESS_BUS_OUT = BIT(33),
    EAU_DATA_BUS_IN     = BIT(32),
    EAU_LO_SELECT       = BIT(31),
    EAU_HI_SELECT       = BIT(30),
    
    LSU_FLAGS_OUT               = BIT(29),
    LSU_WRITE_ENABLE            = BIT(28),
    LSU_READ_ENABLE             = BIT(27),
    LSU_RAM_ENABLE              = BIT(26),
    LSU_ROM_ENABLE              = BIT(25),
    LSU_DECREMENT_STACK_POINTER = BIT(24),
    LSU_INCREMENT_STACK_POINTER = BIT(23),
    
    ALU_ROTATE_RIGHT = BIT(22),
    ALU_ROTATE_LEFT  = BIT(21),
    ALU_NOT          = BIT(20),
    ALU_OR           = BIT(19),
    ALU_AND          = BIT(18),
    ALU_SUBTRACTION  = BIT(17),
    ALU_ADDITION     = BIT(16),
    
    ALU_WRITE_A   = BIT(15),
    ALU_WRITE_B   = BIT(14),
    ALU_WRITE_F   = BIT(13),
    ALU_WRITE_OUT = BIT(12),
    ALU_OUT       = BIT(11),
    ALU_IN        = BIT(10),
    
    RF_FLAGS_OUT = BIT(9),
    RF_D_OUT     = BIT(8),
    RF_C_OUT     = BIT(7),
    RF_B_OUT     = BIT(6),
    RF_A_OUT     = BIT(5),
    
    RF_FLAGS_IN = BIT(4),
    RF_D_IN     = BIT(3),
    RF_C_IN     = BIT(2),
    RF_B_IN     = BIT(1),
    RF_A_IN     = BIT(0),
    
};


static constexpr const auto FETCH = PC_ADDRESS_BUS_OUT | LSU_ROM_ENABLE | LSU_READ_ENABLE | PCU_INSTRUCTION_REGISTER_IN;



std::array<std::uint64_t, 8> emitMvb(std::uint64_t dest_in, std::uint64_t src_out)
{
    return
    {
        FETCH,
        dest_in | src_out,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
        0ull,
        0ull,
        0ull,
        0ull,
    };
}

std::array<std::uint64_t, 8> emitAlu2Arg(std::uint64_t dest_in, std::uint64_t dest_out, std::uint64_t src_out, std::uint64_t op)
{
    return
    {
        FETCH,
        dest_out | ALU_WRITE_A | ALU_IN,
        src_out  | ALU_WRITE_B | ALU_IN,
        RF_FLAGS_OUT | ALU_WRITE_F | ALU_IN,
        op | ALU_IN | ALU_WRITE_OUT,
        ALU_OUT | dest_in,
        PC_SKIP_TO_NEXT_INSTRUCTION,
        0ull,
    };
}





int main(int argc, const char** argv)
{
    switch (argc)
    {
        case 2:
        {
            std::string_view path = argv[1];
            
            std::ofstream file(path, std::ios::out | std::ios::binary);
            
            if (file.good())
            {
                std::array<std::array<std::uint64_t, 8>, std::numeric_limits<std::uint8_t>::max() + 1> µcode{ 0 };
                
                
                
                µcode[MVB_A_B] = emitMvb(RF_A_IN, RF_B_OUT);
                µcode[MVB_A_C] = emitMvb(RF_A_IN, RF_C_OUT);
                µcode[MVB_A_D] = emitMvb(RF_A_IN, RF_D_OUT);
                
                µcode[MVB_B_A] = emitMvb(RF_B_IN, RF_A_OUT);
                µcode[MVB_B_C] = emitMvb(RF_B_IN, RF_C_OUT);
                µcode[MVB_B_D] = emitMvb(RF_B_IN, RF_D_OUT);
                
                µcode[MVB_C_A] = emitMvb(RF_C_IN, RF_A_OUT);
                µcode[MVB_C_B] = emitMvb(RF_C_IN, RF_B_OUT);
                µcode[MVB_C_D] = emitMvb(RF_C_IN, RF_D_OUT);
                
                µcode[MVB_D_A] = emitMvb(RF_D_IN, RF_A_OUT);
                µcode[MVB_D_B] = emitMvb(RF_D_IN, RF_B_OUT);
                µcode[MVB_D_C] = emitMvb(RF_C_IN, RF_C_OUT);
                
                
                
                
                //fetch
                µcode[0] =
                {
                    //PC_ADDRESS_BUS_OUT | LSU_ROM_ENABLE | LSU_READ_ENABLE,
                    0u,
                    0u,
                    0u,
                    0u,
                    
                    0u,
                    0u,
                    0u,
                    0u
                };
                
                µcode[NOP] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
                
                //brk impossible
                µcode[BRK] =
                {
                    IMM(static_cast<std::uint64_t>(0b0001000)) | IMMEDIATE_IN | CONNECT_F_TO_DATA_BUS | RF_FLAGS_IN,
                    PC_SKIP_TO_NEXT_INSTRUCTION,
                    0ull,
                    0ull,
                    
                    0ull,
                    0ull,
                    0ull,
                    0ull
                };
                
                µcode[ADC_A_B] =
                {
                    RF_A_OUT | ALU_WRITE_A | ALU_IN,
                    RF_B_OUT | ALU_WRITE_B | ALU_IN,
                    RF_FLAGS_OUT | ALU_WRITE_F | ALU_IN,
                    ALU_ADDITION | ALU_IN | ALU_WRITE_OUT,
                    ALU_OUT | RF_A_IN,
                    PC_SKIP_TO_NEXT_INSTRUCTION,
                    0ull,
                    0ull,
                };
                
                µcode[MVB_B_A] =
                {
                    RF_B_OUT | RF_A_IN,
                    PC_SKIP_TO_NEXT_INSTRUCTION,
                    0ull,
                    0ull,
                    
                    0ull,
                    0ull,
                    0ull,
                    0ull
                };
                
                
                //jnz
                //and Flags with zero bit
                //load temp reg with 255
                
    
                auto counter = 0;
                
            
                for (auto i = 0; i < µcode.size(); ++i)
                {
                    for (auto j = 0; j < µcode[0].size(); ++j)
                    {
                        auto µop = µcode[i][j];
                        
                        counter++;
                        std::fprintf(stderr, "Counter: %i\n", counter);
                        
                        std::uint32_t hi = ((µop & 0xFFFFFFFF00000000) >> 32);
                        std::uint32_t lo = ((µop & 0x00000000FFFFFFFF) >> 0);
                        
                        std::uint16_t hi_hi = ((hi & 0xFFFF0000) >> 16);
                        std::uint16_t hi_lo = ((hi & 0x0000FFFF) >> 0);
                        
                        std::uint16_t lo_hi = ((lo & 0xFFFF0000) >> 16);
                        std::uint16_t lo_lo = ((lo & 0x0000FFFF) >> 0);
                        
                        
                        

                        std::uint8_t hi_hi_hi = ((hi_hi & 0xFF00) >> 8);
                        std::uint8_t hi_hi_lo = ((hi_hi & 0x00FF) >> 0);
                        
                        std::uint8_t hi_lo_hi = ((hi_lo & 0xFF00) >> 8);
                        std::uint8_t hi_lo_lo = ((hi_lo & 0x00FF) >> 0);
                        
                        
                        std::uint8_t lo_hi_hi = ((lo_hi & 0xFF00) >> 8);
                        std::uint8_t lo_hi_lo = ((lo_hi & 0x00FF) >> 0);
                        
                        std::uint8_t lo_lo_hi = ((lo_lo & 0xFF00) >> 8);
                        std::uint8_t lo_lo_lo = ((lo_lo & 0x00FF) >> 0);
                        
                        using b = std::uint8_t;
                        
#define WRITE(x) file.write(reinterpret_cast<char*>(&x), sizeof(x))
                        
                        
                        WRITE(lo_lo_hi);
                        WRITE(lo_lo_lo);
                        
                        WRITE(lo_hi_hi);
                        WRITE(lo_hi_lo);
                        
                        WRITE(hi_lo_hi);
                        WRITE(hi_lo_lo);
                        
                        WRITE(hi_hi_hi);
                        WRITE(hi_hi_lo);
                        
                        
                        
                        
                        //file.write(reinterpret_cast<char*>(&µop), sizeof(µop));
                    }
                }
                
                //file.write(buffer.data(), buffer.size() * sizeof(*buffer.data()));
            }
            else
            {
                std::fprintf(stderr, "Could not open file %s for writing\n", path.data());
                return EXIT_FAILURE;
            }
        } break;
            
        default:
            std::fprintf(stderr, "Invalid number of arguments specified\n");
            return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
