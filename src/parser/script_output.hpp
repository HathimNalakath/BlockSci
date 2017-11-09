//
//  script_output.hpp
//  blocksci
//
//  Created by Harry Kalodner on 3/18/17.
//
//

#ifndef script_output_hpp
#define script_output_hpp

#include "basic_types.hpp"
#include "script_processor.hpp"
#include "address_state.hpp"
#include "parser_fwd.hpp"

#include <blocksci/scripts/bitcoin_pubkey.hpp>
#include <blocksci/address/address_info.hpp>
#include <blocksci/address/address.hpp>
#include <blocksci/scripts/script_info.hpp>

#include <boost/variant/variant.hpp>

#include <array>

template<auto type>
struct ScriptOutput {
    ScriptData<type> data;
    uint32_t scriptNum;
    bool isNew;
    
    ScriptOutput() = default;
    ScriptOutput(const ScriptData<type> &data_) : data(data_) {}
    
    void resolve(AddressState &state) {
        constexpr auto script = blocksci::scriptType(type);
        if constexpr (blocksci::ScriptInfo<script>::deduped) {
            blocksci::RawScript rawAddress{data.getHash(), script};
            auto addressInfo = state.findAddress(rawAddress);
            std::tie(scriptNum, isNew) = state.resolveAddress(addressInfo);
        } else {
            scriptNum = state.getNewAddressIndex(scriptType(type));
            isNew = true;
        }
        
        if (isNew) {
            data.resolve(state);
        }
    }
    
    void check(const AddressState &state) {
        constexpr auto script = blocksci::scriptType(type);
        if constexpr (blocksci::ScriptInfo<script>::deduped) {
            blocksci::RawScript rawAddress{data.getHash(), script};
            auto addressInfo = state.findAddress(rawAddress);
            scriptNum = addressInfo.addressNum;
            isNew = addressInfo.addressNum == 0;
        } else {
            scriptNum = 0;
            isNew = true;
        }
        
        data.check(state);
    }
};

struct ScriptDataBase {
    void resolve(AddressState &) {}
    void check(const AddressState &) {}
    bool isValid() const { return true; }
};

template <>
struct ScriptData<blocksci::AddressType::Enum::PUBKEY> : public ScriptDataBase {
    CPubKey pubkey;
    
    ScriptData(const boost::iterator_range<const unsigned char *> &vch1);
    ScriptData(const CPubKey &pub) : pubkey(pub) {}
    ScriptData() = default;
    
    blocksci::uint160 getHash() const;
};

template <>
struct ScriptData<blocksci::AddressType::Enum::PUBKEYHASH> : public ScriptDataBase {
    
    CKeyID hash;
    
    ScriptData(blocksci::uint160 &pubkeyHash) : hash{pubkeyHash} {}
    
    blocksci::uint160 getHash() const;
};

template <>
struct ScriptData<blocksci::AddressType::Enum::WITNESS_PUBKEYHASH> : public ScriptDataBase {
    
    CKeyID hash;
    
    ScriptData(blocksci::uint160 &&pubkeyHash) : hash{pubkeyHash} {}
    
    blocksci::uint160 getHash() const;
};

template <>
struct ScriptData<blocksci::AddressType::Enum::SCRIPTHASH> : public ScriptDataBase {
    CKeyID hash;
    
    ScriptData(blocksci::uint160 hash_) : hash(hash_) {}
    
    blocksci::uint160 getHash() const;
    
};

template <>
struct ScriptData<blocksci::AddressType::Enum::WITNESS_SCRIPTHASH> : public ScriptDataBase {
    blocksci::uint256 hash;
    
    ScriptData(blocksci::uint256 hash_) : hash(hash_) {}
    
    blocksci::uint160 getHash() const;
    
};

template <>
struct ScriptData<blocksci::AddressType::Enum::MULTISIG> : public ScriptDataBase {
    static constexpr int MAX_ADDRESSES = 16;
    uint8_t numRequired;
    uint8_t numTotal;
    uint16_t addressCount;
    
    std::vector<ScriptOutput<blocksci::AddressType::Enum::PUBKEY>> addresses;
    
    ScriptData() : addressCount(0) {}
    
    void addAddress(const boost::iterator_range<const unsigned char *> &vch1);
    
    bool isValid() const {
        return numRequired <= numTotal && numTotal == addressCount;
    }
    
    blocksci::uint160 getHash() const;
    void resolve(AddressState &state);
    void check(const AddressState &state);
};

template <>
struct ScriptData<blocksci::AddressType::Enum::NONSTANDARD> : public ScriptDataBase {
    CScriptView script;
    
    ScriptData() {}
    ScriptData(const CScriptView &script);
};

template <>
struct ScriptData<blocksci::AddressType::Enum::NULL_DATA> : public ScriptDataBase {
    std::vector<unsigned char> fullData;
    
    ScriptData(const CScriptView &script);
};

using ScriptOutputType = blocksci::to_address_variant_t<ScriptOutput>;

class AnyScriptOutput {
public:
    ScriptOutputType wrapped;
    blocksci::Address address() const;
    bool isNew() const;
    blocksci::AddressType::Enum type() const;
    
    AnyScriptOutput() = default;
    AnyScriptOutput(const CScriptView &scriptPubKey, bool witnessActivated);
    
    void check(const AddressState &state);
    void resolve(AddressState &state);
    bool isValid() const;
};

#endif /* script_output_hpp */
