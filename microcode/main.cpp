#include <cstdint>
#include <cstdlib>

#include <format>

#include <fstream>
#include <array>
#include <string>
#include <string_view>
#include <limits>

#include "opcode.h"

namespace
{
    consteval auto BIT(unsigned int s) noexcept
    {
        return (std::uint64_t{ 1 } << s);
    }
    
    consteval auto LEN(unsigned int s) noexcept
    {
        //this is a bit hacky
        return (std::uint64_t{ s & 2 } << 62);
    }
    
    static constexpr const auto µcode_line_width = 8;

    using µcode_type = std::uint64_t;
    using µcode_line = std::array<µcode_type, µcode_line_width>;
    
    enum : µcode_type
    {
        // register file
        RF_AI = BIT(0),
        RF_BI = BIT(1),
        RF_CI = BIT(2),
        RF_DI = BIT(3),
        RF_FI = BIT(4),
        
        RF_AO = BIT(5),
        RF_BO = BIT(6),
        RF_CO = BIT(7),
        RF_DO = BIT(8),
        RF_FO = BIT(9),
        // /register file
    
        // load store unit
        LSU_RE = BIT(10),
        LSU_WE = BIT(11),
        LSU_SP_D = BIT(12), //stack pointer direction
        LSU_SP_WE = BIT(13),
        LSU_SP_EN = BIT(14),
        // /load store unit
        
        // arithmetic logic unit
        ALU_ADD = BIT(15),
        ALU_SUB = BIT(16),
        ALU_AND = BIT(17),
        ALU_OR = BIT(18),
        ALU_NOT = BIT(19),
        ALU_SHL = BIT(20),
        ALU_SHR = BIT(21),
        ALU_WA = BIT(22),
        ALU_WB = BIT(23),
        ALU_OE = BIT(24),
        // /arithmetic logic unit
        
        // instruction register
        IR_WE = BIT(25),
        // /instruction register
        
        // program counter
        PC_LRC = BIT(26),
        PC_INI = BIT(27),
        PC_CUB = BIT(28),
        PC_OE = BIT(29),
        // /program counter
        
        // address composition unit
        ACU_WL = BIT(30),
        ACU_WH = BIT(31),
        ACU_OE = BIT(32),
        // /address composition unit
        
        // address decomposition unit
        ADU_RL = BIT(33),
        ADU_RH = BIT(34),
        ADU_OE = BIT(35),
        // /address decomposition unit
    };
    
    static constexpr const auto FETCH_INSTRUCTION = PC_OE | LSU_RE | IR_WE;
    
    //determine how the instruction register will be written to after a reset to begin
    
    //maybe--on reset
    // PC_OE | PC_LRC | LSU_RE | IR_WE
    
    // Pseudocode:
    // output pc
    // input pc as load & reset -> step = 0 again (hopefully)
    // read from rom
    // write to ir
    
    
    
    
    
    //register to register transfer
    µcode_line mvb_data(µcode_type dest_in, µcode_type src_out) noexcept
    {
        return
        {
            FETCH_INSTRUCTION,
            src_out | dest_in,
            PC_SKIP_TO_NEXT_INSTRUCTION,
            0ull,
            
            0ull,
            0ull,
            0ull,
            0ull,
        };
    }
    
    
    
}



