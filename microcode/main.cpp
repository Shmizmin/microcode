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

enum : std::uint64_t
{
    PC_LOAD_VALUE               = BIT(38),
    PC_ENABLE                   = BIT(37),
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

int main(int argc, const char** argv)
{
    switch (argc)
    {
        case 3:
        {
            std::string_view command = argv[1], path = argv[2];
            
            std::ofstream file(path, std::ios::out | std::ios::binary);
            
            if (file.good())
            {
                if (command == std::string_view("-c"))
                {
                    struct operation
                    {
                        std::array<std::uint64_t, 8> sequence;
                    };
                    
                    std::array<operation, std::numeric_limits<std::uint8_t>::max()> microcode;
                    
                    //fetch
                    microcode[NULL] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
                    
                    microcode[NOP] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
                    
                    //brk impossible
                    
                    microcode[ADC_A_B] =
                    {
                        RF_A_OUT | ALU_WRITE_A | ALU_IN,
                        RF_B_OUT | ALU_WRITE_B | ALU_IN,
                        RF_FLAGS_OUT | ALU_WRITE_F | ALU_IN,
                        ALU_ADDITION | ALU_IN,
                        ALU_ADDITION | ALU_IN | ALU_WRITE_OUT,
                        ALU_OUT | RF_A_IN,
                        
                        
                    };
                    
                    //file.write(buffer.data(), buffer.size() * sizeof(*buffer.data()));
                }
                else
                {
                    std::fprintf(stderr, "Invalid command\n");
                    return EXIT_FAILURE;
                }
                
                
            }
            else
            {
                std::fprintf(stderr, "Could not open file %s for writing\n", path.data());
                return EXIT_FAILURE;
            }
        }
            
        default:
            std::fprintf(stderr, "Invalid number of arguments specified\n");
            return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
