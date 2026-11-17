import ifj26.*

def ev(n: Int): Int =
  if n == 0 then
    return 1
  else
    return od(n - 1)

def od(n: Int): Int =
  if n == 0 then
    return 0
  else
    return ev(n - 1)

@main
def main(): Unit =
  print(toStr(ev(10)) + toStr(od(7)) + toStr(ev(7)))
