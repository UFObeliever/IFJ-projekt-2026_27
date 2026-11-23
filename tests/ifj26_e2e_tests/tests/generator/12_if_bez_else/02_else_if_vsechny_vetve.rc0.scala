import ifj26.*

def f(x: Int): Unit =
  if x < 0 then
    print("zaporne ")
  else if x == 0 then
    print("nula ")
  else if x < 10 then
    print("male ")
  else
    print("velke ")

@main def main(): Unit =
  f(0 - 1)
  f(0)
  f(5)
  f(50)
