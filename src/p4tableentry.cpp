#include "p4tableentry.hpp"

#include "logger.hpp"

using namespace std;

P4TableEntry::P4TableEntry(const string &tableName, const string &actionName)
    : _tableName(tableName), _actionName(actionName) {}

P4TableEntry::P4TableEntry(const p4::v1::TableEntry &entry,
                           pi_p4info_t *p4info) {
    pi_p4_id_t tableId = entry.table_id();
    this->_tableName = pi_p4info_table_name_from_id(p4info, tableId);

    for (const auto &mf : entry.match()) {
        pi_p4_id_t fieldId = mf.field_id();
        string fieldName =
            pi_p4info_table_match_field_name_from_id(p4info, tableId, fieldId);
        size_t index =
            pi_p4info_table_match_field_index(p4info, tableId, fieldId);
        auto info = pi_p4info_table_match_field_info(p4info, tableId, index);
        vector<string> matchValues;
        matchValues.reserve(2);

        switch (info->match_type) {
        case PI_P4INFO_MATCH_TYPE_EXACT: {
            matchValues.emplace_back(mf.exact().value());
            break;
        }
        case PI_P4INFO_MATCH_TYPE_LPM: {
            matchValues.emplace_back(mf.lpm().value());
            matchValues.emplace_back(to_string(mf.lpm().prefix_len()));
            break;
        }
        case PI_P4INFO_MATCH_TYPE_TERNARY: {
            matchValues.emplace_back(mf.ternary().value());
            matchValues.emplace_back(mf.ternary().mask());
            break;
        }
        case PI_P4INFO_MATCH_TYPE_RANGE: {
            matchValues.emplace_back(mf.range().low());
            matchValues.emplace_back(mf.range().high());
            break;
        }
        default:
            logger.error("Match type " + to_string(info->match_type) +
                         " is not supported");
        }

        this->_matchFields.emplace(std::move(fieldName),
                                   std::move(matchValues));
    }

    pi_p4_id_t actionId = entry.action().action().action_id();
    this->_actionName = pi_p4info_action_name_from_id(p4info, actionId);

    for (const auto &param : entry.action().action().params()) {
        string paramName = pi_p4info_action_param_name_from_id(
            p4info, actionId, param.param_id());
        this->_actionParams.emplace(std::move(paramName), param.value());
    }

    this->_priority = entry.priority();
}

void P4TableEntry::addMatchField(const string &name, vector<string> &&values) {
    this->_matchFields.emplace(name, values);
}

void P4TableEntry::addActionParam(const string &name, const string &value) {
    this->_actionParams.emplace(name, value);
}

p4::v1::TableEntry P4TableEntry::protobufMsg(pi_p4info_t *p4info) const {
    p4::v1::TableEntry entry;

    pi_p4_id_t tableId =
        pi_p4info_table_id_from_name(p4info, this->_tableName.c_str());
    entry.set_table_id(tableId);

    for (const auto &[fieldName, matchValues] : this->_matchFields) {
        auto mf = entry.add_match();
        pi_p4_id_t fieldId = pi_p4info_table_match_field_id_from_name(
            p4info, tableId, fieldName.c_str());
        mf->set_field_id(fieldId);
        size_t index =
            pi_p4info_table_match_field_index(p4info, tableId, fieldId);
        auto info = pi_p4info_table_match_field_info(p4info, tableId, index);

        switch (info->match_type) {
        case PI_P4INFO_MATCH_TYPE_EXACT: {
            assert(matchValues.size() == 1);
            auto exact = mf->mutable_exact();
            exact->set_value(matchValues[0]);
            break;
        }
        case PI_P4INFO_MATCH_TYPE_LPM: {
            assert(matchValues.size() == 2);
            auto lpm = mf->mutable_lpm();
            lpm->set_value(matchValues[0]);
            lpm->set_prefix_len(stoi(matchValues[1]));
            break;
        }
        case PI_P4INFO_MATCH_TYPE_TERNARY: {
            assert(matchValues.size() == 2);
            auto ternary = mf->mutable_ternary();
            ternary->set_value(matchValues[0]);
            ternary->set_mask(matchValues[1]);
            break;
        }
        case PI_P4INFO_MATCH_TYPE_RANGE: {
            assert(matchValues.size() == 2);
            auto range = mf->mutable_range();
            range->set_low(matchValues[0]);
            range->set_high(matchValues[1]);
            break;
        }
        default:
            logger.error("Match type " + to_string(info->match_type) +
                         " is not supported");
        }
    }

    auto action = entry.mutable_action()->mutable_action();
    pi_p4_id_t actionId =
        pi_p4info_action_id_from_name(p4info, this->_actionName.c_str());
    action->set_action_id(actionId);

    for (const auto &[paramName, paramValue] : this->_actionParams) {
        auto param = action->add_params();
        auto paramId = pi_p4info_action_param_id_from_name(p4info, actionId,
                                                           paramName.c_str());
        param->set_param_id(paramId);
        param->set_value(paramValue);
    }

    entry.set_priority(this->_priority);

    return entry;
}
