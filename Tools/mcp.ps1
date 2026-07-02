# Minimal MCP (Model Context Protocol) JSON-RPC client for the UE editor server.
# Usage:
#   .\mcp.ps1 list                          # list tools
#   .\mcp.ps1 call <toolName> '<jsonArgs>'  # call a tool
param(
    [Parameter(Mandatory=$true)][string]$Command,
    [string]$Tool,
    [string]$JsonArgs = '{}',
    [string]$Url = 'http://localhost:8000/mcp'
)

$headers = @{ 'Content-Type' = 'application/json'; 'Accept' = 'application/json, text/event-stream' }

function Invoke-Rpc([hashtable]$Body) {
    $json = $Body | ConvertTo-Json -Depth 12 -Compress
    $resp = Invoke-WebRequest -Uri $Url -Method Post -Headers $headers -Body $json -UseBasicParsing
    $text = $resp.Content
    # Server may answer as SSE ("data: {...}") or plain JSON
    if ($text -match '(?m)^data:\s*(.+)$') { $text = $Matches[1] }
    return $text
}

switch ($Command) {
    'init' {
        Invoke-Rpc @{ jsonrpc='2.0'; id=1; method='initialize'; params=@{ protocolVersion='2025-03-26'; capabilities=@{}; clientInfo=@{ name='claude-cli'; version='1.0' } } }
    }
    'list' {
        Invoke-Rpc @{ jsonrpc='2.0'; id=2; method='tools/list'; params=@{} }
    }
    'call' {
        $parsed = $JsonArgs | ConvertFrom-Json
        $argTable = @{}
        if ($parsed) { $parsed.PSObject.Properties | ForEach-Object { $argTable[$_.Name] = $_.Value } }
        Invoke-Rpc @{ jsonrpc='2.0'; id=3; method='tools/call'; params=@{ name=$Tool; arguments=$argTable } }
    }
    default { Write-Error "Unknown command: $Command (use init|list|call)" }
}
