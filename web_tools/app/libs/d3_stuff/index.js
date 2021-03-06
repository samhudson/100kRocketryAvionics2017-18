const EXTRA_ANGLE = 15,
    whRatio = 2 / (Math.sin(deg2rad(EXTRA_ANGLE)) + 1.1),
    r = Math.min(window.innerWidth, window.innerHeight * whRatio) / 2,
    angleScale = d3.scaleLinear().domain([0, 100]).range([-90 - EXTRA_ANGLE, 90 + EXTRA_ANGLE])

// Size canvas
const svg = d3.select('#canvas')
    .attr('width', r * 2)
    .attr('height', r * 2 * whRatio)
    .attr('viewBox', `${-r} ${-r} ${r*2} ${r*2*whRatio}`)

// Add axis
svg.append('g').classed('axis', true)
    .call(d3.axisRadialInner(
        angleScale.copy().range(angleScale.range().map(deg2rad)),
        r - 5
    ))

// Add needle
const needle = svg.append('g')
    .attr('transform', `scale(${r * 0.85})`)
    .append('path').classed('needle', true)
        .attr('d', ['M0 -1', 'L0.03 0', 'A 0.03 0.03 0 0 1 -0.03 0', 'Z'].join(' '))
        .attr('transform', `rotate(${angleScale(0)})`)

// Add val label
const label = svg.append('text').classed('label', true)
    .attr('x', 0)
    .attr('y', r * 0.2)
    .attr('text-anchor', 'middle')
    .text('0')

function setPosition(value) {
    const newVal = Math.round(value)
    label.text(newVal)
    needle.transition().duration(500).ease(d3.easeBackIn)
        .attr('transform', `rotate(${angleScale(newVal)})`)
}


function deg2rad(deg) { return deg * Math.PI / 180 }
setInterval(() => {
    setPosition(Math.random() * (60 - 40) + 40);
}, 1000)