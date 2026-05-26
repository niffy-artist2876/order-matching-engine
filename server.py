import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import time

st.title("Order Book Dashboard")

placeholder = st.empty()

while True:
    df = pd.read_csv('build/trades.csv')
    df = df[df['bid'] > 0].reset_index(drop=True)

    with placeholder.container():
        fig_bidask = go.Figure()
        fig_bidask.add_trace(go.Scatter(y=df['bid'], name='Bid', line=dict(color='#00cc44')))
        fig_bidask.add_trace(go.Scatter(y=df['ask'], name='Ask', line=dict(color='#ff4444')))
        LAYOUT = dict(height=450, margin=dict(l=40, r=40, t=50, b=40))

        fig_bidask.update_layout(title='Bid / Ask', yaxis_title='Price', **LAYOUT)
        st.plotly_chart(fig_bidask, use_container_width=True)
        st.markdown("<br>", unsafe_allow_html=True)

        fig_mid = go.Figure()
        fig_mid.add_trace(go.Scatter(y=(df['bid'] + df['ask']) / 2, name='Mid', line=dict(color='#3366ff')))
        fig_mid.update_layout(title='Mid Price', yaxis_title='Price', **LAYOUT)
        st.plotly_chart(fig_mid, use_container_width=True)
        st.markdown("<br>", unsafe_allow_html=True)

        fig_trade = go.Figure()
        fig_trade.add_trace(go.Scatter(y=df['trade_price'], name='Trade Price', line=dict(color='#9933cc')))
        fig_trade.update_layout(title='Trade Price', yaxis_title='Price', **LAYOUT)
        st.plotly_chart(fig_trade, use_container_width=True)
        st.markdown("<br>", unsafe_allow_html=True)

        fig_inv = go.Figure()
        fig_inv.add_trace(go.Scatter(y=df['inventory'], name='Inventory', line=dict(color='#ff8800'), fill='tozeroy'))
        fig_inv.update_layout(title='Inventory', yaxis_title='Inventory', **LAYOUT)
        st.plotly_chart(fig_inv, use_container_width=True)

    time.sleep(1)
