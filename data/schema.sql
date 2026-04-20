-- 1. SECTORS
CREATE TABLE
    sectors (
        sector_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
        sector_name VARCHAR(32) NOT NULL UNIQUE
    );

-- 2. INDUSTRIES
CREATE TABLE
    industries (
        industry_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
        industry_name VARCHAR(64) NOT NULL UNIQUE,
        sector_id INTEGER NOT NULL,
        CONSTRAINT fk_industries_sector FOREIGN KEY (sector_id) REFERENCES sectors (sector_id)
    );

-- 3. UNIVERSE
CREATE TABLE
    universe (
        security_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
        ticker VARCHAR(16) NOT NULL,
        industry_id INTEGER,
        active_flag BOOLEAN NOT NULL DEFAULT TRUE,
        start_date DATE NOT NULL DEFAULT CURRENT_DATE,
        end_date DATE,
        CONSTRAINT fk_universe_industry FOREIGN KEY (industry_id) REFERENCES industries (industry_id),
        CONSTRAINT chk_universe_dates CHECK (
            end_date IS NULL
            OR end_date >= start_date
        )
    );

-- Useful, but not unique because the same ticker can exist across lifecycle periods.
CREATE INDEX idx_universe_ticker ON universe (ticker);

-- Helps reduce accidental duplicate lifecycle rows.
CREATE UNIQUE INDEX uq_universe_ticker_start_date ON universe (ticker, start_date);

-- 4. INDICES
CREATE TABLE
    indices (
        index_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
        index_name VARCHAR(32) NOT NULL UNIQUE
    );

-- 5. INDEX CONSTITUENTS
CREATE TABLE
    index_constituents (
        index_id INTEGER NOT NULL,
        security_id INTEGER NOT NULL,
        start_date DATE NOT NULL,
        end_date DATE,
        PRIMARY KEY (index_id, security_id, start_date),
        CONSTRAINT fk_index_constituents_index FOREIGN KEY (index_id) REFERENCES indices (index_id),
        CONSTRAINT fk_index_constituents_security FOREIGN KEY (security_id) REFERENCES universe (security_id),
        CONSTRAINT chk_index_constituents_dates CHECK (
            end_date IS NULL
            OR end_date >= start_date
        )
    );

-- 6. PRICES DAILY
CREATE TABLE
    prices_daily (
        date DATE NOT NULL,
        security_id INTEGER NOT NULL,
        open NUMERIC(18, 6) NOT NULL DEFAULT 0,
        high NUMERIC(18, 6) NOT NULL DEFAULT 0,
        low NUMERIC(18, 6) NOT NULL DEFAULT 0,
        close NUMERIC(18, 6) NOT NULL DEFAULT 0,
        adj_close NUMERIC(18, 6) NOT NULL DEFAULT 0,
        volume BIGINT NOT NULL DEFAULT 0,
        PRIMARY KEY (date, security_id),
        CONSTRAINT fk_prices_daily_security FOREIGN KEY (security_id) REFERENCES universe (security_id),
        CONSTRAINT chk_prices_nonnegative CHECK (
            open >= 0
            AND high >= 0
            AND low >= 0
            AND close >= 0
            AND adj_close >= 0
            AND volume >= 0
        ),
        CONSTRAINT chk_prices_ohlc_bounds CHECK (
            high >= low
            AND high >= open
            AND high >= close
            AND low <= open
            AND low <= close
        )
    );

-- 7. OPTIONS SNAPSHOT
CREATE TABLE
    options_snapshot (
        date DATE NOT NULL,
        security_id INTEGER NOT NULL,
        spot NUMERIC(18, 6) NOT NULL DEFAULT 0,
        iv30 REAL DEFAULT 0,
        iv_percentile REAL,
        atm_call_iv REAL,
        atm_put_iv REAL,
        term_slope REAL,
        skew_proxy REAL,
        put_call_ratio REAL,
        PRIMARY KEY (date, security_id),
        CONSTRAINT fk_options_snapshot_security FOREIGN KEY (security_id) REFERENCES universe (security_id),
        CONSTRAINT fk_options_snapshot_price FOREIGN KEY (date, security_id) REFERENCES prices_daily (date, security_id),
        CONSTRAINT chk_options_values CHECK (
            spot >= 0
            AND (
                iv30 IS NULL
                OR iv30 >= 0
            )
            AND (
                iv_percentile IS NULL
                OR (
                    iv_percentile >= 0
                    AND iv_percentile <= 1
                )
            )
            AND (
                atm_call_iv IS NULL
                OR atm_call_iv >= 0
            )
            AND (
                atm_put_iv IS NULL
                OR atm_put_iv >= 0
            )
            AND (
                put_call_ratio IS NULL
                OR put_call_ratio >= 0
            )
        )
    );

-- 8. FUNDAMENTALS QUARTERLY
CREATE TABLE
    fundamentals_quarterly (
        security_id INTEGER NOT NULL,
        period_end DATE NOT NULL,
        filing_date DATE NOT NULL,
        revenue NUMERIC(20, 2) NOT NULL DEFAULT 0,
        eps REAL DEFAULT 0,
        gross_margin REAL DEFAULT 0,
        operating_margin REAL DEFAULT 0,
        fcf NUMERIC(20, 2),
        debt NUMERIC(20, 2),
        shares_outstanding BIGINT NOT NULL DEFAULT 0,
        PRIMARY KEY (security_id, period_end, filing_date),
        CONSTRAINT fk_fundamentals_security FOREIGN KEY (security_id) REFERENCES universe (security_id),
        CONSTRAINT chk_fundamentals_dates CHECK (filing_date >= period_end),
        CONSTRAINT chk_fundamentals_nonnegative CHECK (
            revenue >= 0
            AND shares_outstanding >= 0
            AND (
                debt IS NULL
                OR debt >= 0
            )
        )
    );

-- 9. FEATURES MONTHLY
CREATE TABLE
    features_monthly (
        snapshot_date DATE NOT NULL,
        security_id INTEGER NOT NULL,
        mom_1m REAL,
        mom_3m REAL,
        mom_6m REAL,
        mom_12m_ex1 REAL,
        vol_20d DOUBLE PRECISION,
        drawdown_252d REAL,
        rel_strength_sector REAL,
        quality_score REAL,
        valuation_score REAL,
        PRIMARY KEY (snapshot_date, security_id),
        CONSTRAINT fk_features_security FOREIGN KEY (security_id) REFERENCES universe (security_id),
        CONSTRAINT chk_features_vol CHECK (
            vol_20d IS NULL
            OR vol_20d >= 0
        )
    );

-- 10. LABELS MONTHLY
CREATE TABLE
    labels_monthly (
        snapshot_date DATE NOT NULL,
        security_id INTEGER NOT NULL,
        fwd_1m_ret REAL,
        fwd_3m_ret REAL,
        fwd_6m_ret REAL,
        fwd_3m_excess_spy REAL,
        top_decile_flag BOOLEAN,
        PRIMARY KEY (snapshot_date, security_id),
        CONSTRAINT fk_labels_security FOREIGN KEY (security_id) REFERENCES universe (security_id)
    );

-- 11. DATA SOURCES
CREATE TABLE
    data_sources (
        source_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
        source_name VARCHAR(64) NOT NULL UNIQUE
    );

-- Time-series indexes
CREATE INDEX idx_prices_daily_date_brin ON prices_daily USING BRIN (date);

CREATE INDEX idx_options_snapshot_date_brin ON options_snapshot USING BRIN (date);

-- Ticker/history access
CREATE INDEX idx_prices_daily_security_date ON prices_daily (security_id, date DESC);

CREATE INDEX idx_options_snapshot_security_date ON options_snapshot (security_id, date DESC);

CREATE INDEX idx_features_monthly_security_date ON features_monthly (security_id, snapshot_date DESC);

CREATE INDEX idx_labels_monthly_security_date ON labels_monthly (security_id, snapshot_date DESC);

-- Universe joins
CREATE INDEX idx_universe_industry_id ON universe (industry_id);

-- Historical index membership lookups
CREATE INDEX idx_index_constituents_index_security_dates ON index_constituents (index_id, security_id, start_date, end_date);

CREATE INDEX idx_index_constituents_dates ON index_constituents (start_date, end_date);
