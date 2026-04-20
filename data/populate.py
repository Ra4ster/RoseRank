import pandas as pd
import yfinance as yf

tickers = ["NVDA", "AAPL", "MSFT", "KO", "TSLA", "SPY"]

data = yf.download(
    tickers=tickers,
    start="2026-04-10",
    end="2026-04-18",
    auto_adjust=False,
    progress=False
)

close = data["Close"].dropna(how="all")
close = close.reset_index()

long_df = close.melt(id_vars="Date", var_name="ticker", value_name="close")
long_df.columns = ["date", "ticker", "close"]

long_df.to_csv("price_history.csv", index=False)
print(long_df.tail(20))