#!/bin/bash

# Test UI API to verify error messages are fixed
cd /home/hades/chamber_of_secrets/Compiler-Design-Project

echo "Testing UI API with fixed parser..."
echo ""

# Test with OOP file that had false positive
code=$(cat test/oop/access_modifiers.c)

payload=$(cat <<'EOF'
{
    "code": "class Base {\n    private:\n    int secret;\n    public:\n    void set(int s) {\n        secret = s;\n    }\n};\n\nint main() {\n    Base b;\n    b.set(10);\n    b.secret = 20;\n    return 0;\n}",
    "optimizationLevel": 0,
    "useMetrics": false
}
EOF
)

echo "Testing OOP class definition error response..."
echo "Expected: Parser error WITHOUT false positive hint about keywords"
echo ""

# Make API request to compile endpoint
response=$(curl -s -X POST http://127.0.0.1:3000/api/compile \
    -H "Content-Type: application/json" \
    -d "$payload" 2>&1)

if [ -z "$response" ]; then
    echo "⚠ WARNING: No response from server (may not be running)"
else
    echo "Response received from UI API:"
    echo "$response" | jq '.stderr' 2>/dev/null | head -20
    
    # Check if the response contains the false positive hint
    if echo "$response" | jq '.stderr' 2>/dev/null | grep -i "looks similar to keyword"; then
        echo "❌ FAILED: False positive hint still present in UI response"
    else
        echo "✅ PASSED: No false positive hint in UI response"
    fi
fi
