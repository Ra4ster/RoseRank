import pandas as pd
import yfinance as yf
from datetime import datetime

OUTPUT_FILE = "stocks_to_watch.csv"

TOP_50 = [
    "AAPL", "MSFT", "NVDA", "AMZN", "GOOGL", "META", "BRK-B", "TSLA", "AVGO", "LLY",
    "JPM", "WMT", "XOM", "V", "MA", "UNH", "COST", "JNJ", "ORCL", "HD",
    "PG", "ABBV", "BAC", "NFLX", "KO", "CRM", "CVX", "MRK", "AMD", "TMO",
    "PEP", "ACN", "CSCO", "WFC", "MCD", "ABT", "LIN", "IBM", "GE", "DIS",
    "ADBE", "PM", "CAT", "TXN", "INTU", "QCOM", "AMGN", "NOW", "GS", "SPGI"
]

def year_fraction(expiry_str: str) -> float:
    today = datetime.now().date()
    expiry = datetime.strptime(expiry_str, "%Y-%m-%d").date()
    return max((expiry - today).days / 365.0, 0.0)

def midpoint(bid, ask, fallback=None):
    if pd.notna(bid) and pd.notna(ask) and bid > 0 and ask > 0:
        return (bid + ask) / 2.0
    if fallback is not None and pd.notna(fallback):
        return float(fallback)
    return 0.0

def choose_expiry(expiries):
    if not expiries:
        return None
    today = datetime.now().date()
    for exp in expiries:
        d = datetime.strptime(exp, "%Y-%m-%d").date()
        if (d - today).days >= 7:
            return exp
    return expiries[0]

def choose_atm_strike(option_df: pd.DataFrame, spot: float):
    if option_df.empty:
        return None
    idx = (option_df["strike"] - spot).abs().idxmin()
    return float(option_df.loc[idx, "strike"])

def placeholder_rank(index: int, total: int) -> float:
    # Temporary market-cap proxy rank: top ticker gets 1.00, last gets 0.02
    return round((total - index) / total, 2)

def fetch_row(ticker: str, index: int, total: int):
    tk = yf.Ticker(ticker)

    info = tk.fast_info
    spot = info.get("lastPrice", None)
    if spot is None:
        hist = tk.history(period="5d")
        if hist.empty:
            raise ValueError(f"No price history for {ticker}")
        spot = float(hist["Close"].iloc[-1])

    expiries = tk.options
    expiry = choose_expiry(expiries)
    if expiry is None:
        raise ValueError(f"No expiries for {ticker}")

    chain = tk.option_chain(expiry)
    calls = chain.calls.copy()
    puts = chain.puts.copy()

    if calls.empty or puts.empty:
        raise ValueError(f"Incomplete option chain for {ticker}")

    strike = choose_atm_strike(calls, spot)
    if strike is None:
        raise ValueError(f"No ATM strike found for {ticker}")

    call_row = calls.loc[(calls["strike"] - strike).abs().idxmin()]
    put_row = puts.loc[(puts["strike"] - strike).abs().idxmin()]

    call_price = midpoint(call_row.get("bid"), call_row.get("ask"), call_row.get("lastPrice"))
    put_price = midpoint(put_row.get("bid"), put_row.get("ask"), put_row.get("lastPrice"))
    expiry_t = round(year_fraction(expiry), 4)

    return {
        "Ticker": ticker,
        "Stock_Rank": placeholder_rank(index, total),
        "Current_Price": round(float(spot), 2),
        "Call_Price": round(float(call_price), 2),
        "Put_Price": round(float(put_price), 2),
        "Strike": round(float(strike), 2),
        "Expiry_T": expiry_t,
        "Hist_IV_Avg": 0.25,  # placeholder for now
        "Hist_IV_Std": 0.05,  # placeholder for now
    }

def main():
    rows = []
    total = len(TOP_50)

    for i, ticker in enumerate(TOP_50):
        try:
            out = fetch_row(ticker, i, total)
            rows.append(out)
            print(f"Updated {out['Ticker']}: spot={out['Current_Price']}, strike={out['Strike']}, T={out['Expiry_T']}")
        except Exception as e:
            print(f"Failed {ticker}: {e}")

    out_df = pd.DataFrame(rows)
    out_df.to_csv(OUTPUT_FILE, index=False)
    print(f"\nWrote {len(out_df)} rows to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()