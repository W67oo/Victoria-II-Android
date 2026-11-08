$path1 = "src/dcon/common_types.hpp"
$lines1 = [System.IO.File]::ReadAllLines($path1)
$newLines1 = [System.Collections.Generic.List[string]]::new()
for ($i = 0; $i -lt $lines1.Length; $i++) {
    if ($lines1[$i] -match "^\s*inline void bit_vector_set\(bitfield_type\* v, int32_t index, bool value\) \{") {
        $newLines1.Add($lines1[$i])
        $newLines1.Add($lines1[$i+1])
        $newLines1.Add($lines1[$i+2])
        $newLines1.Add("#if defined(__cpp_lib_atomic_ref)")
        $newLines1.Add("		if(value)")
        $newLines1.Add("			std::atomic_ref(v[real_index].v).fetch_or(uint8_t(uint32_t(1) << sub_index));")
        $newLines1.Add("		else")
        $newLines1.Add("			std::atomic_ref(v[real_index].v).fetch_and(uint8_t(~(uint32_t(1) << sub_index)));")
        $newLines1.Add("#elif defined(_MSC_VER)")
        $newLines1.Add("		if(value)")
        $newLines1.Add("			std::atomic_ref(v[real_index].v).fetch_or(uint8_t(uint32_t(1) << sub_index));")
        $newLines1.Add("		else")
        $newLines1.Add("			std::atomic_ref(v[real_index].v).fetch_and(uint8_t(~(uint32_t(1) << sub_index)));")
        $newLines1.Add("#else")
        $newLines1.Add("		if(value)")
        $newLines1.Add("			__atomic_fetch_or(&v[real_index].v, uint8_t(uint32_t(1) << sub_index), __ATOMIC_SEQ_CST);")
        $newLines1.Add("		else")
        $newLines1.Add("			__atomic_fetch_and(&v[real_index].v, uint8_t(~(uint32_t(1) << sub_index)), __ATOMIC_SEQ_CST);")
        $newLines1.Add("#endif")
        $newLines1.Add("	}")
        $i += 7
    } else {
        $newLines1.Add($lines1[$i])
    }
}
[System.IO.File]::WriteAllLines($path1, $newLines1, [System.Text.UTF8Encoding]::new($false))
Write-Host "Patched common_types.hpp"

$path2 = "src/engine/scripting/fif.hpp"
$lines2 = [System.IO.File]::ReadAllLines($path2)
$newLines2 = [System.Collections.Generic.List[string]]::new()
for ($i = 0; $i -lt $lines2.Length; $i++) {
    if ($lines2[$i] -match "^\s*inline bool float_from_chars\(char const\* start, char const\* end, float& float_out\) \{") {
        $newLines2.Add("inline bool float_from_chars(char const* start, char const* end, float& float_out) { // returns true on success")
        $newLines2.Add("	int32_t magnitude = 0;")
        $newLines2.Add("	int64_t accumulated = 0;")
        $newLines2.Add("	bool after_decimal = false;")
        $newLines2.Add("	if(start >= end) return false;")
        $newLines2.Add("	bool is_negative = false;")
        $newLines2.Add("	if(*start == '-') { is_negative = true; ++start; }")
        $newLines2.Add("	else if(*start == '+') { ++start; }")
        $newLines2.Add("	for(; start < end; ++start) {")
        $newLines2.Add("		if(*start >= '0' && *start <= '9') {")
        $newLines2.Add("			accumulated = accumulated * 10 + (*start - '0');")
        $newLines2.Add("			magnitude += int32_t(after_decimal);")
        $newLines2.Add("		} else if(!after_decimal && *start == '.') {")
        $newLines2.Add("			after_decimal = true;")
        $newLines2.Add("		} else { return false; }")
        $newLines2.Add("	}")
        $newLines2.Add("	if(!is_negative) {")
        $newLines2.Add("		if(magnitude > 0 && magnitude < 309) float_out = float(double(accumulated) / pow_10(magnitude));")
        $newLines2.Add("		else float_out = float(accumulated);")
        $newLines2.Add("	} else {")
        $newLines2.Add("		if(magnitude > 0 && magnitude < 309) float_out = -float(double(accumulated) / pow_10(magnitude));")
        $newLines2.Add("		else float_out = -float(accumulated);")
        $newLines2.Add("	}")
        $newLines2.Add("	return true;")
        $newLines2.Add("}")
        $i += 5 # skip the original 6-line function
    } else {
        $newLines2.Add($lines2[$i])
    }
}
[System.IO.File]::WriteAllLines($path2, $newLines2, [System.Text.UTF8Encoding]::new($false))
Write-Host "Patched fif.hpp"
