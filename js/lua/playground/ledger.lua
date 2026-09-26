-- A tiny double-entry ledger. Money is integer cents, never floats:
print("0.1 + 0.2 == 0.3 ?", 0.1 + 0.2 == 0.3)

local entries = {}

-- Every posting moves money between two accounts: one debit, one credit.
local function post(ref, debit, credit, cents)
	assert(math.type(cents) == "integer", ref .. ": amount must be integer cents")
	entries[#entries + 1] = { ref = ref, debit = debit, credit = credit, cents = cents }
end

local function money(cents)
	return string.format("%s%d.%02d", cents < 0 and "-" or "", math.abs(cents) // 100, math.abs(cents) % 100)
end

-- A checkout for 50.00, the provider keeps 1.75, pays out the rest.
post("ORD-1001", "provider_receivable", "sales", 5000)
post("ORD-1001", "fees", "provider_receivable", 175)
post("PAYOUT-7", "bank", "provider_receivable", 4825)
-- A 12.00 refund on an earlier order.
post("ORD-0998", "sales", "bank", 1200)

local balances, debits, credits = {}, 0, 0
for _, e in ipairs(entries) do
	balances[e.debit] = (balances[e.debit] or 0) + e.cents
	balances[e.credit] = (balances[e.credit] or 0) - e.cents
	debits, credits = debits + e.cents, credits + e.cents
end

local names = {}
for name in pairs(balances) do names[#names + 1] = name end
table.sort(names)
for _, name in ipairs(names) do
	print(string.format("%-20s %10s", name, money(balances[name])))
end

-- The invariant: debits equal credits, and the receivable is fully paid out.
assert(debits == credits, "ledger does not balance")
assert(balances.provider_receivable == 0, "provider still owes " .. money(balances.provider_receivable))
print(string.format("balanced: %s debits = %s credits", money(debits), money(credits)))
